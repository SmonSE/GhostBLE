#include "lora_scanner.h"

#include <atomic>
#include <map>
#include <mutex>
#include <SD.h>

#include "app/context/scan_context.h"
#include "app/context/device_context.h"
#include "app/context/network_context.h"
#include "app/context/globals.h"
#include "app/context/ui_context.h"
#include "app/interaction/nibbles_speech.h"
#include "infrastructure/logging/logger.h"        // braucht Kategorie LOG_LORA
#include "infrastructure/platform/hardware_config.h"
#include "ui/expression/show_expression.h"
#include "ui/menu/menu_controller.h"
#include "ui/tasks/ui_expression_tasks.h"


#if defined(LORA_CS_PIN)

#include <RadioLib.h>
#include <SPI.h>

// Zusätzlich zu LORA_CS_PIN in hardware_config.h definieren
#if !defined(LORA_DIO1_PIN) || !defined(LORA_RST_PIN) || !defined(LORA_BUSY_PIN)
  #error "LORA_DIO1_PIN / LORA_RST_PIN / LORA_BUSY_PIN in hardware_config.h ergaenzen"
#endif

// 1 = zusätzlich jedes Paket als CSV auf die SD schreiben, 0 = aus
#ifndef LORA_CSV_LOG
  #define LORA_CSV_LOG 1
#endif

namespace LoraScanner {

enum class Kind : uint8_t { MESH = 0, LORAWAN = 1 };

// ---------------------------------------------------------------------------
//  Funkprofile
// ---------------------------------------------------------------------------
// Meshtastic EU868 LongFast — Werte bitte gegen die aktuelle Doku prüfen
static constexpr float    MESH_FREQ_MHZ  = 869.525f;
static constexpr float    MESH_BW_KHZ    = 250.0f;
static constexpr uint8_t  MESH_SF        = 11;
static constexpr uint8_t  MESH_SYNC      = 0x2B;
static constexpr uint16_t MESH_PREAMBLE  = 16;

// LoRaWAN EU868, nur Uplinks (normales IQ, öffentliches Sync Word).
// Das Radio hört immer nur EINE Kombination aus Kanal und SF. Es hüpft deshalb
// alle LW_DWELL_MS durch diese Liste. Mehr Kombinationen = weniger Zeit pro Kombination.
// Zusätzliche TTN-Kanäle: 867.1, 867.3, 867.5, 867.7, 867.9 MHz.
static constexpr float    LW_FREQS_MHZ[] = { 868.1f, 868.3f, 868.5f };
static constexpr uint8_t  LW_SFS[]       = { 7 }; // { 7, 8, 9 };
static constexpr float    LW_BW_KHZ      = 125.0f;
static constexpr uint8_t  LW_SYNC        = 0x34;
static constexpr uint16_t LW_PREAMBLE    = 8;
static constexpr uint32_t LW_DWELL_MS    = 3000;

// Im Profil AUTO bleibt das Radio so lange auf Meshtastic, bevor die LoRaWAN-Runde startet
static constexpr uint32_t AUTO_MESH_DWELL_MS = 20000;

static constexpr uint8_t  LORA_CR        = 5;       // 4/5
static constexpr int8_t   LORA_POWER_DBM = 10;      // nur relevant fürs Senden

static constexpr uint32_t CYCLE_MS        = 5000;   // Verarbeitungszyklus
static constexpr size_t   MAX_NODES       = 128;   // ca. 15-20 KB Heap; bei Überlauf fliegt der älteste Node raus
static constexpr size_t   MESH_HEADER     = 16;     // Klartext-Header eines Meshtastic-Pakets
static constexpr size_t   DEDUP_RING      = 64;     // gemerkte (from, packetId)-Paare
static constexpr uint32_t BROADCAST_ADDR  = 0xFFFFFFFF;
static constexpr size_t   CSV_FLUSH_BYTES = 6000;
static constexpr size_t   MAX_SLOTS       = 32;

struct RadioSlot {
    Kind     kind;
    float    freqMhz;
    float    bwKhz;
    uint8_t  sf;
    uint32_t dwellMs;      // 0 = bleiben (nur bei genau einem Slot)
};

static RadioSlot schedule[MAX_SLOTS];
static size_t    scheduleLen = 0;
static size_t    slotIdx     = 0;
static uint32_t  nextHopAt   = 0;

static Kind      currentKind = Kind::MESH;
static float     currentFreq = MESH_FREQ_MHZ;
static uint8_t   currentSf   = MESH_SF;

static std::atomic<uint8_t> wantedProfile{ (uint8_t)RadioProfile::AUTO };
static RadioProfile         usedProfile = RadioProfile::AUTO;

// ---------------------------------------------------------------------------
//  Zustand
// ---------------------------------------------------------------------------
static SX1262*                   radio = nullptr;
static volatile bool             rxFlag = false;
static bool                      active = false;
static bool                      scannedSinceDump = false;
static uint32_t                  trackedNode = 0;

// Schlüssel: Meshtastic = Node-ID, LoRaWAN = (1<<32) | DevAddr.
// Wird vom scanTask geschrieben und von der UI (Liste, Stats) gelesen.
static std::map<uint64_t, Node>  nodes;
static std::mutex                nodesMutex;

// Zähler pro Zyklus
static uint32_t cyclePackets   = 0;
static uint32_t cycleDirect    = 0;
static uint32_t cycleRelayed   = 0;
static uint32_t cycleDup       = 0;
static uint32_t cycleCrcErrors = 0;
static uint32_t cycleShort     = 0;
static uint32_t cycleJoin      = 0;
static uint32_t cycleOther     = 0;
static uint32_t cycleAirUs     = 0;
static uint32_t irqPolled      = 0;   // Diagnose: RX_DONE per Polling statt DIO1-Interrupt

// Zähler seit Boot
static uint32_t totalPackets   = 0;
static uint32_t totalDirect    = 0;
static uint32_t totalCrcErrors = 0;

// Duplikaterkennung (Meshtastic): dasselbe Paket kommt über Relays mehrfach an
struct SeenPkt { uint32_t from; uint32_t id; };
static SeenPkt seenRing[DEDUP_RING] = {};
static size_t  seenPos = 0;

// Alles, was wir über ein empfangenes Meshtastic-Paket wissen
struct PacketInfo {
    const char*    type      = "OK";     // "OK", "CRC", "SHORT", "ERR"
    bool           hasHeader = false;
    uint32_t       from = 0, to = 0, id = 0;
    uint8_t        hopLimit = 0, hopStart = 0;
    int            hopsUsed = -1;        // -1 = unbekannt (hop_start == 0, ältere Firmware)
    bool           wantAck = false, viaMqtt = false, dup = false;
    uint8_t        channel = 0, nextHop = 0, relay = 0;
    float          rssi = 0, snr = 0;
    size_t         len = 0;
    uint32_t       toaUs = 0;
    const uint8_t* raw = nullptr;
};

// Alles, was wir über ein empfangenes LoRaWAN-Paket wissen (nur der Klartext-Teil)
struct LwInfo {
    const char*    type      = "OK";     // "OK", "CRC", "ERR", "OTHER"
    uint8_t        mtype     = 0xFF;     // 0..7, 0xFF = unbekannt
    const char*    mtypeName = "?";
    bool           isJoin = false, hasAddr = false, hasDelta = false, hasPort = false;
    uint32_t       devAddr = 0;
    uint64_t       devEui = 0, joinEui = 0;
    uint16_t       devNonce = 0;
    uint16_t       fcnt = 0;
    int            delta = 0;            // Differenz zum letzten Frame-Counter
    int            missed = 0;           // vermutlich nicht gehörte Frames
    uint8_t        fport = 0;
    bool           adr = false, ack = false;
    uint8_t        foptsLen = 0;
    int            payloadLen = 0;
    float          freqMhz = 0, rssi = 0, snr = 0;
    uint8_t        sf = 0;
    size_t         len = 0;
    uint32_t       toaUs = 0;
    const uint8_t* raw = nullptr;
};

struct GpsFix {
    bool     valid = false;
    double   lat = 0, lon = 0, alt = 0;
    unsigned sats = 0;
    String   time;
};

static void IRAM_ATTR onPacket() { rxFlag = true; }

// ---------------------------------------------------------------------------
//  Helper
// ---------------------------------------------------------------------------
static uint32_t readLE32(const uint8_t* p) {
    return  (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static uint64_t readLE64(const uint8_t* p) {
    return (uint64_t)readLE32(p) | ((uint64_t)readLE32(p + 4) << 32);
}

static String nodeIdToString(uint32_t id) {
    char b[12];
    snprintf(b, sizeof(b), "!%08x", (unsigned)id);
    return String(b);
}

static String hex2(uint8_t v) {
    char b[4];
    snprintf(b, sizeof(b), "%02X", v);
    return String(b);
}

static String hex4(uint16_t v) {
    char b[8];
    snprintf(b, sizeof(b), "%04X", (unsigned)v);
    return String(b);
}

static String hex8(uint32_t v) {
    char b[12];
    snprintf(b, sizeof(b), "%08X", (unsigned)v);
    return String(b);
}

static String hex32(uint32_t v) {
    char b[12];
    snprintf(b, sizeof(b), "0x%08x", (unsigned)v);
    return String(b);
}

// EUI-64 so, wie sie in der TTN-Konsole stehen (höchstwertiges Byte zuerst)
static String eui64ToString(uint64_t v) {
    char b[20];
    snprintf(b, sizeof(b), "%08X%08X", (unsigned)(v >> 32), (unsigned)(v & 0xFFFFFFFFu));
    return String(b);
}

static String toHex(const uint8_t* d, size_t n) {
    String out;
    out.reserve(n * 3);
    for (size_t i = 0; i < n; i++) {
        if (d[i] < 0x10) out += "0";
        out += String(d[i], HEX);
        out += " ";
    }
    out.toUpperCase();
    return out;
}

static String toHexCompact(const uint8_t* d, size_t n) {
    String out;
    out.reserve(n * 2);
    for (size_t i = 0; i < n; i++) {
        if (d[i] < 0x10) out += "0";
        out += String(d[i], HEX);
    }
    out.toUpperCase();
    return out;
}

static const char* lwTypeName(uint8_t t) {
    switch (t) {
        case 0:  return "JOIN REQ";
        case 1:  return "JOIN ACC";
        case 2:  return "UNCONF UP";
        case 3:  return "UNCONF DOWN";
        case 4:  return "CONF UP";
        case 5:  return "CONF DOWN";
        case 6:  return "REJOIN";
        default: return "PROPRIETARY";
    }
}

// Aktuell eingestellter Slot, z. B. "MESH 869.525 MHz SF11" oder "LW 868.1 MHz SF7"
static String slotLabel() {
    return String(currentKind == Kind::LORAWAN ? "LW " : "MESH ")
         + String(currentFreq, 3) + " MHz SF" + String(currentSf);
}

// Nur mit gehaltenem nodesMutex aufrufen
static void evictOldest() {
    auto oldest = nodes.begin();
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (it->second.lastSeen < oldest->second.lastSeen) oldest = it;
    }
    if (oldest != nodes.end()) nodes.erase(oldest);
}

// true, wenn (from, id) kürzlich schon gehört wurde; merkt sich das Paar sonst
static bool isDuplicate(uint32_t from, uint32_t id) {
    for (size_t i = 0; i < DEDUP_RING; i++) {
        if (seenRing[i].from == from && seenRing[i].id == id) return true;
    }
    seenRing[seenPos] = { from, id };
    seenPos = (seenPos + 1) % DEDUP_RING;
    return false;
}

// GPS-Daten wie im Rest der App: nur mit aktivem Wardriving und gültigem Fix
static GpsFix readGps() {
    GpsFix g;
    if (NetworkContext::wardrivingEnabled.load() && NetworkContext::gpsManager.isValid()) {
        g.valid = true;
        g.lat   = NetworkContext::gpsManager.getLatitude();
        g.lon   = NetworkContext::gpsManager.getLongitude();
        g.alt   = NetworkContext::gpsManager.getAltitude();
        g.sats  = (unsigned)NetworkContext::gpsManager.getSatellites();
        g.time  = String(NetworkContext::gpsManager.getTimestamp().c_str());
    }
    return g;
}

// ---------------------------------------------------------------------------
//  CSV-Logs (gepuffert, pro Zyklus in einem Rutsch geschrieben)
//    lora_packets.csv    = Meshtastic
//    lorawan_packets.csv = LoRaWAN
// ---------------------------------------------------------------------------
struct CsvSink {
    const char* path;
    const char* header;
    String      buf;
};

static CsvSink csvMesh = {
    "/GhostBLE/lora_packets.csv",
    "ms,gps_time,type,from,to,packet_id,rssi_dbm,snr_db,len,toa_us,"
    "hop_limit,hop_start,hops_used,want_ack,via_mqtt,channel,next_hop,relay,dup,"
    "lat,lon,alt,sats,raw",
    String()
};

static CsvSink csvLw = {
    "/GhostBLE/lorawan_packets.csv",
    "ms,gps_time,type,mtype,devaddr,dev_eui,join_eui,dev_nonce,fcnt,fcnt_delta,fport,"
    "adr,ack,fopts_len,payload_len,freq_mhz,sf,bw_khz,rssi_dbm,snr_db,len,toa_us,"
    "lat,lon,alt,sats,raw",
    String()
};

static void csvFlush(CsvSink& s) {
#if LORA_CSV_LOG
    if (s.buf.isEmpty()) return;

    const bool isNewFile = !SD.exists(s.path);
    File f = SD.open(s.path, FILE_APPEND);
    if (f) {
        if (isNewFile) {
            f.print(s.header);
            f.print('\n');
        }
        f.print(s.buf);
        f.close();
    }
    s.buf.clear();   // auch bei Fehler leeren, damit der Heap nicht wächst
#else
    (void)s;
#endif
}

static void csvFlushAll() {
    csvFlush(csvMesh);
    csvFlush(csvLw);
}

static void csvAppend(CsvSink& s, const String& line) {
#if LORA_CSV_LOG
    s.buf += line;
    if (s.buf.length() > CSV_FLUSH_BYTES) csvFlush(s);
#else
    (void)s; (void)line;
#endif
}

static void csvAdd(const PacketInfo& p, const GpsFix& g, uint32_t now) {
#if LORA_CSV_LOG
    auto col = [](String& line, const String& v) { line += v; line += ','; };
    auto opt = [](bool has, const String& v) { return has ? v : String(""); };

    String l;
    l.reserve(220 + p.len * 2);

    col(l, String(now));
    col(l, g.valid ? g.time : String(""));
    col(l, String(p.type));
    col(l, opt(p.hasHeader, nodeIdToString(p.from)));
    col(l, opt(p.hasHeader, nodeIdToString(p.to)));
    col(l, opt(p.hasHeader, hex32(p.id)));
    col(l, String(p.rssi, 0));
    col(l, String(p.snr, 1));
    col(l, String((unsigned)p.len));
    col(l, String(p.toaUs));
    col(l, opt(p.hasHeader, String(p.hopLimit)));
    col(l, opt(p.hasHeader, String(p.hopStart)));
    col(l, opt(p.hasHeader && p.hopsUsed >= 0, String(p.hopsUsed)));
    col(l, opt(p.hasHeader, String(p.wantAck ? 1 : 0)));
    col(l, opt(p.hasHeader, String(p.viaMqtt ? 1 : 0)));
    col(l, opt(p.hasHeader, hex2(p.channel)));
    col(l, opt(p.hasHeader, hex2(p.nextHop)));
    col(l, opt(p.hasHeader, hex2(p.relay)));
    col(l, opt(p.hasHeader, String(p.dup ? 1 : 0)));
    col(l, g.valid ? String(g.lat, 6) : String(""));
    col(l, g.valid ? String(g.lon, 6) : String(""));
    col(l, g.valid ? String(g.alt, 1) : String(""));
    col(l, g.valid ? String(g.sats)   : String(""));
    l += toHexCompact(p.raw, p.len);
    l += '\n';

    csvAppend(csvMesh, l);
#else
    (void)p; (void)g; (void)now;
#endif
}

static void csvAddLw(const LwInfo& p, const GpsFix& g, uint32_t now) {
#if LORA_CSV_LOG
    auto col = [](String& line, const String& v) { line += v; line += ','; };
    auto opt = [](bool has, const String& v) { return has ? v : String(""); };

    String l;
    l.reserve(240 + p.len * 2);

    col(l, String(now));
    col(l, g.valid ? g.time : String(""));
    col(l, String(p.type));
    col(l, p.mtype <= 7 ? String(p.mtypeName) : String(""));
    col(l, opt(p.hasAddr, hex8(p.devAddr)));
    col(l, opt(p.isJoin, eui64ToString(p.devEui)));
    col(l, opt(p.isJoin, eui64ToString(p.joinEui)));
    col(l, opt(p.isJoin, hex4(p.devNonce)));
    col(l, opt(p.hasAddr, String(p.fcnt)));
    col(l, opt(p.hasDelta, String(p.delta)));
    col(l, opt(p.hasPort, String(p.fport)));
    col(l, opt(p.hasAddr, String(p.adr ? 1 : 0)));
    col(l, opt(p.hasAddr, String(p.ack ? 1 : 0)));
    col(l, opt(p.hasAddr, String(p.foptsLen)));
    col(l, opt(p.hasAddr, String(p.payloadLen)));
    col(l, String(p.freqMhz, 3));
    col(l, String(p.sf));
    col(l, String(LW_BW_KHZ, 0));
    col(l, String(p.rssi, 0));
    col(l, String(p.snr, 1));
    col(l, String((unsigned)p.len));
    col(l, String(p.toaUs));
    col(l, g.valid ? String(g.lat, 6) : String(""));
    col(l, g.valid ? String(g.lon, 6) : String(""));
    col(l, g.valid ? String(g.alt, 1) : String(""));
    col(l, g.valid ? String(g.sats)   : String(""));
    l += toHexCompact(p.raw, p.len);
    l += '\n';

    csvAppend(csvLw, l);
#else
    (void)p; (void)g; (void)now;
#endif
}

// CSV-Zeile für Fehlpakete (CRC, Lesefehler) in die Datei des aktuellen Protokolls
static void csvAddError(const char* type, float rssi, float snr, size_t len, uint32_t toaUs,
                        const uint8_t* raw, const GpsFix& gps, uint32_t now) {
    if (currentKind == Kind::LORAWAN) {
        LwInfo p;
        p.type = type; p.rssi = rssi; p.snr = snr; p.len = len; p.toaUs = toaUs; p.raw = raw;
        p.freqMhz = currentFreq; p.sf = currentSf;
        csvAddLw(p, gps, now);
    } else {
        PacketInfo p;
        p.type = type; p.rssi = rssi; p.snr = snr; p.len = len; p.toaUs = toaUs; p.raw = raw;
        csvAdd(p, gps, now);
    }
}

// ---------------------------------------------------------------------------
//  Node-Tabelle ins Log (beim Stoppen des Scans)
// ---------------------------------------------------------------------------
static void dumpNodeTable() {
    String out;
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(nodesMutex);
        count = nodes.size();
        for (const auto& kv : nodes) {
            const Node& n = kv.second;

            if (n.lorawan) {
                out += "\n   LW " + hex8(n.id)
                     + " | pkts " + String(n.packets)
                     + " | missed ~" + String(n.missedFrames)
                     + " | FPort " + String(n.lastFport);
            } else {
                out += "\n   " + nodeIdToString(n.id)
                     + " | pkts " + String(n.packets)
                     + " (direct " + String(n.direct) + " / relayed " + String(n.relayed) + ")";
            }

            if (n.rssiSamples > 0) {
                out += " | RSSI " + String(n.rssi)
                     + " [" + String(n.rssiMin) + ".." + String(n.rssiMax) + "]"
                     + " avg " + String(n.rssiSum / (int32_t)n.rssiSamples)
                     + " | SNR " + String(n.snr, 1);
            } else {
                out += " | RSSI n/a (nur ueber Relays gehoert)";
            }

            if (n.lorawan) {
                out += " | " + String(n.freqMhz, 1) + " MHz SF" + String(n.sf);
            } else {
                out += " | ch 0x" + hex2(n.channel);
            }

            out += " | first " + String(n.firstSeen / 1000) + "s"
                 + " last " + String(n.lastSeen / 1000) + "s";

            if (n.hasPos) {
                out += " | best pos " + String(n.lat, 6) + "," + String(n.lon, 6);
            }
        }
    }

    LOG(LOG_LORA, "Node table (" + String((unsigned)count) + " nodes, "
        + String(totalPackets) + " packets since boot):" + out);
}

// ---------------------------------------------------------------------------
//  Profile und Slots
// ---------------------------------------------------------------------------
static void buildSchedule(RadioProfile p) {
    scheduleLen = 0;

    if (p == RadioProfile::MESHTASTIC || p == RadioProfile::AUTO) {
        const uint32_t dwell = (p == RadioProfile::AUTO) ? AUTO_MESH_DWELL_MS : 0u;
        schedule[scheduleLen++] = { Kind::MESH, MESH_FREQ_MHZ, MESH_BW_KHZ, MESH_SF, dwell };
    }

    if (p == RadioProfile::LORAWAN || p == RadioProfile::AUTO) {
        for (float f : LW_FREQS_MHZ) {
            for (uint8_t sf : LW_SFS) {
                if (scheduleLen >= MAX_SLOTS) break;
                schedule[scheduleLen++] = { Kind::LORAWAN, f, LW_BW_KHZ, sf, LW_DWELL_MS };
            }
        }
    }
    slotIdx = 0;
}

// Radio komplett neu auf einen Slot einstellen und Empfang starten.
// begin() setzt alle Parameter konsistent (CRC an, normales IQ).
static bool configureSlot(const RadioSlot& s) {
    const bool mesh = (s.kind == Kind::MESH);

    radio->clearPacketReceivedAction();

    int16_t st = radio->begin(s.freqMhz, s.bwKhz, s.sf, LORA_CR,
                              mesh ? MESH_SYNC : LW_SYNC, LORA_POWER_DBM,
                              mesh ? MESH_PREAMBLE : LW_PREAMBLE);
    if (st != RADIOLIB_ERR_NONE) {
        // Typisch: Modul nicht gesteckt (Chip antwortet nicht) oder falsche TCXO-Einstellung
        LOG(LOG_SYSTEM, "LoRa init failed, code " + String(st));
        return false;
    }

    radio->setDio2AsRfSwitch(true);              // modulabhängig, ggf. entfernen
    radio->setPacketReceivedAction(onPacket);

    st = radio->startReceive();
    if (st != RADIOLIB_ERR_NONE) {
        LOG(LOG_SYSTEM, "LoRa startReceive failed, code " + String(st));
        return false;
    }

    rxFlag      = false;
    currentKind = s.kind;
    currentFreq = s.freqMhz;
    currentSf   = s.sf;
    return true;
}

static void hopToNext() {
    slotIdx = (slotIdx + 1) % scheduleLen;
    if (!configureSlot(schedule[slotIdx])) {
        nextHopAt = millis() + 1000;             // später erneut versuchen
        return;
    }
    nextHopAt = millis() + schedule[slotIdx].dwellMs;
}

// Profilwechsel von der UI wird hier im scanTask umgesetzt (kein Radio-Zugriff aus loop())
static void applyPendingProfile() {
    const RadioProfile wanted = (RadioProfile)wantedProfile.load();
    if (wanted == usedProfile) return;

    usedProfile = wanted;
    buildSchedule(usedProfile);
    if (configureSlot(schedule[0])) {
        nextHopAt = millis() + schedule[0].dwellMs;
        LOG(LOG_SYSTEM, String("LoRa profile: ") + profileName(usedProfile));
    }
}

// ---------------------------------------------------------------------------
//  Meshtastic-Paket verarbeiten
// ---------------------------------------------------------------------------
static void handleMesh(const uint8_t* buf, size_t len, float rssi, float snr,
                       uint32_t toaUs, uint32_t now, const GpsFix& gps) {
    PacketInfo p;
    p.rssi  = rssi;
    p.snr   = snr;
    p.len   = len;
    p.toaUs = toaUs;
    p.raw   = buf;

    // --- Zu kurz für Meshtastic-Header: anderes Protokoll oder Müll ---
    if (len < MESH_HEADER) {
        cycleShort++;
        p.type = "SHORT";
        LOG(LOG_LORA, "[SHORT / NON-MESH] " + String((unsigned)len) + " B | RSSI " + String(rssi, 0)
            + " dBm | SNR " + String(snr, 1) + " dB\n   raw: " + toHex(buf, len));
        csvAdd(p, gps, now);
        return;
    }

    // --- Header auswerten (Layout laut Meshtastic-Doku, bitte gegenprüfen) ---
    // dest(4) | from(4) | packetId(4) | flags(1) | channelHash(1) | nextHop(1) | relayNode(1)
    // nextHop/relayNode gibt es erst bei neuerer Firmware, davor waren es reservierte Bytes.
    p.hasHeader = true;
    p.to        = readLE32(buf + 0);
    p.from      = readLE32(buf + 4);
    p.id        = readLE32(buf + 8);

    const uint8_t flags = buf[12];
    p.hopLimit = flags & 0x07;
    p.wantAck  = (flags & 0x08) != 0;
    p.viaMqtt  = (flags & 0x10) != 0;
    p.hopStart = (flags >> 5) & 0x07;
    p.channel  = buf[13];
    p.nextHop  = buf[14];
    p.relay    = buf[15];
    p.hopsUsed = (p.hopStart > 0 && p.hopStart >= p.hopLimit)
                     ? (int)(p.hopStart - p.hopLimit) : -1;
    p.dup      = isDuplicate(p.from, p.id);

    const String idStr = nodeIdToString(p.from);

    // --- Node-Tabelle aktualisieren (Lock nur kurz, Logging läuft danach ohne) ---
    bool isNew = false;
    {
        std::lock_guard<std::mutex> lock(nodesMutex);

        const uint64_t key = (uint64_t)p.from;
        auto it = nodes.find(key);
        isNew = (it == nodes.end());
        if (isNew) {
            if (nodes.size() >= MAX_NODES) evictOldest();
            it = nodes.emplace(key, Node{}).first;
            it->second.id        = p.from;
            it->second.lorawan   = false;
            it->second.firstSeen = now;
        }

        Node& n    = it->second;
        n.lastSeen = now;
        n.packets++;
        n.channel  = p.channel;
        n.lastHops = (p.hopsUsed >= 0) ? (uint8_t)p.hopsUsed : 0xFF;

        if (p.hopsUsed == 0)      n.direct++;
        else if (p.hopsUsed > 0)  n.relayed++;

        // Bei weitergeleiteten Paketen gehört RSSI/SNR zum Relay, nicht zum Absender
        if (p.hopsUsed <= 0) {
            const int16_t r = (int16_t)rssi;
            const bool newBest = (n.rssiSamples == 0) || (r > n.rssiMax);

            if (n.rssiSamples == 0) {
                n.rssiMin = r;
                n.rssiMax = r;
            } else {
                if (r < n.rssiMin) n.rssiMin = r;
                if (r > n.rssiMax) n.rssiMax = r;
            }
            n.rssi = r;
            n.snr  = snr;
            n.rssiSum += r;
            n.rssiSamples++;

            if (newBest && gps.valid) {
                n.hasPos = true;
                n.lat    = gps.lat;
                n.lon    = gps.lon;
            }
        }
    }

    // --- Zähler ---
    cyclePackets++;
    totalPackets++;
    if (p.dup) cycleDup++;
    if (p.hopsUsed == 0)     { cycleDirect++; totalDirect++; }
    else if (p.hopsUsed > 0) { cycleRelayed++; }

    // --- Log-Eintrag: alles, was wir wissen ---
    const int    sid     = ScanContext::getOrAssignDeviceId(std::string(idStr.c_str()));
    const bool   tracked = (p.from == trackedNode && trackedNode != 0);
    const String dest    = (p.to == BROADCAST_ADDR) ? String("BROADCAST") : nodeIdToString(p.to);

    String hopTxt;
    if (p.hopsUsed < 0) {
        hopTxt = "hops ? (hop_start 0)";
    } else {
        hopTxt = "hops " + String(p.hopsUsed) + "/" + String(p.hopStart)
               + (p.hopsUsed == 0 ? " (direct)" : " (relayed)");
    }

    String entry = "[#" + String(sid) + "] "
        + (tracked ? "[TRACK] " : "")
        + (isNew ? "NEW " : "")
        + idStr + " -> " + dest
        + (p.dup ? "  [DUP]" : "")
        + "\n   [t=" + String(now / 1000.0f, 1) + "s]"
        + " RSSI " + String(rssi, 0) + " dBm"
        + (p.hopsUsed > 0 ? " (of relay)" : "")
        + " | SNR " + String(snr, 1) + " dB"
        + " | " + String((unsigned)len) + " B"
        + " | ToA " + String(toaUs / 1000) + " ms"
        + "\n   ID " + hex32(p.id)
        + " | " + hopTxt
        + " | CH 0x" + hex2(p.channel)
        + " | ACK " + String(p.wantAck ? 1 : 0)
        + " | MQTT " + String(p.viaMqtt ? 1 : 0)
        + " | relay 0x" + hex2(p.relay)
        + " | nh 0x" + hex2(p.nextHop);

    if (gps.valid) {
        entry += "\n   GPS " + String(gps.lat, 6) + ", " + String(gps.lon, 6)
               + " | alt " + String(gps.alt, 0) + " m | sat " + String(gps.sats);
    }
    entry += "\n   raw: " + toHex(buf, len);

    LOG(LOG_LORA, entry);
    csvAdd(p, gps, now);

    // --- Neuer Node: XP, Zähler, Nibbles mit Brille + Node-ID in der Sprechblase ---
    if (isNew) {
        ScanContext::allSpottedDevice++;
        DeviceContext::xpManager.awardXP(0.5f);

        displayName = idStr;   // globaler String, die Glasses-Task zeigt ihn an
        if (!UIContext::isAngryTaskRunning.load()) {
            UIExpressionTasks::showIfNotRunning(showGlassesExpressionTask, "LoraGlasses",
                UIContext::isGlassesTaskRunning, UIContext::glassesTaskHandle, 4096, 4, 1);
        }
    }
}

// ---------------------------------------------------------------------------
//  LoRaWAN-Uplink verarbeiten (nur Klartext-Teil, Nutzdaten sind verschlüsselt)
//
//  Data-Uplink:  MHDR(1) | DevAddr(4) | FCtrl(1) | FCnt(2) | FOpts(0..15) |
//                [FPort(1) | FRMPayload(n)] | MIC(4)
//  Join-Request: MHDR(1) | JoinEUI(8) | DevEUI(8) | DevNonce(2) | MIC(4)
//  (Layout laut LoRaWAN-Spezifikation, bitte gegenprüfen)
// ---------------------------------------------------------------------------
static void handleLorawan(const uint8_t* buf, size_t len, float rssi, float snr,
                          uint32_t toaUs, uint32_t now, const GpsFix& gps) {
    LwInfo p;
    p.rssi    = rssi;
    p.snr     = snr;
    p.len     = len;
    p.toaUs   = toaUs;
    p.raw     = buf;
    p.freqMhz = currentFreq;
    p.sf      = currentSf;

    // --- Plausibilitätsprüfung: ist das überhaupt ein LoRaWAN-Frame? ---
    bool valid = false;
    if (len >= 1) {
        const uint8_t mhdr  = buf[0];
        const uint8_t mtype = mhdr >> 5;
        const uint8_t major = mhdr & 0x03;
        p.mtype     = mtype;
        p.mtypeName = lwTypeName(mtype);

        if (major == 0) {
            if (mtype == 0 && len == 23) {
                valid       = true;
                p.isJoin    = true;
                p.joinEui   = readLE64(buf + 1);
                p.devEui    = readLE64(buf + 9);
                p.devNonce  = (uint16_t)(buf[17] | (buf[18] << 8));
            } else if ((mtype == 2 || mtype == 4) && len >= 12) {
                const uint8_t fctrl    = buf[5];
                const uint8_t foptsLen = fctrl & 0x0F;
                if (len >= (size_t)(8 + foptsLen + 4)) {
                    valid        = true;
                    p.hasAddr    = true;
                    p.devAddr    = readLE32(buf + 1);
                    p.adr        = (fctrl & 0x80) != 0;
                    p.ack        = (fctrl & 0x20) != 0;
                    p.foptsLen   = foptsLen;
                    p.fcnt       = (uint16_t)(buf[6] | (buf[7] << 8));

                    const size_t macEnd  = len - 4;               // ohne MIC
                    const size_t portPos = 8 + foptsLen;
                    if (macEnd > portPos) {
                        p.hasPort    = true;
                        p.fport      = buf[portPos];
                        p.payloadLen = (int)(macEnd - portPos - 1);
                    } else {
                        p.payloadLen = 0;
                    }
                }
            }
        }
    }

    // --- Kein gültiger Frame: anderes Protokoll oder Rauschen ---
    if (!valid) {
        cycleOther++;
        p.type = "OTHER";
        LOG(LOG_LORA, "[LW? OTHER] " + slotLabel() + " | " + String((unsigned)len) + " B | RSSI "
            + String(rssi, 0) + " dBm | SNR " + String(snr, 1) + " dB\n   raw: " + toHex(buf, len));
        csvAddLw(p, gps, now);
        return;
    }

    // --- Join-Request: Gerät meldet sich am Netz an (DevEUI/JoinEUI im Klartext) ---
    if (p.isJoin) {
        cyclePackets++;
        totalPackets++;
        cycleDirect++;
        totalDirect++;
        cycleJoin++;

        String entry = "[LW JOIN REQUEST] " + slotLabel()
            + "\n   [t=" + String(now / 1000.0f, 1) + "s]"
            + " RSSI " + String(rssi, 0) + " dBm | SNR " + String(snr, 1) + " dB"
            + " | " + String((unsigned)len) + " B | ToA " + String(toaUs / 1000) + " ms"
            + "\n   DevEUI " + eui64ToString(p.devEui)
            + " | JoinEUI " + eui64ToString(p.joinEui)
            + " | DevNonce 0x" + hex4(p.devNonce);
        if (gps.valid) {
            entry += "\n   GPS " + String(gps.lat, 6) + ", " + String(gps.lon, 6)
                   + " | alt " + String(gps.alt, 0) + " m | sat " + String(gps.sats);
        }
        entry += "\n   raw: " + toHex(buf, len);

        LOG(LOG_LORA, entry);
        csvAddLw(p, gps, now);
        return;
    }

    // --- Daten-Uplink: Node-Tabelle aktualisieren ---
    bool isNew = false;
    {
        std::lock_guard<std::mutex> lock(nodesMutex);

        const uint64_t key = (1ULL << 32) | (uint64_t)p.devAddr;
        auto it = nodes.find(key);
        isNew = (it == nodes.end());
        if (isNew) {
            if (nodes.size() >= MAX_NODES) evictOldest();
            it = nodes.emplace(key, Node{}).first;
            it->second.id        = p.devAddr;
            it->second.lorawan   = true;
            it->second.firstSeen = now;
        }

        Node& n    = it->second;
        n.lastSeen = now;
        n.packets++;
        n.direct++;                               // Uplinks hören wir direkt vom Gerät
        n.lastFport = p.fport;
        n.freqMhz   = p.freqMhz;
        n.sf        = p.sf;

        const int16_t r = (int16_t)rssi;
        const bool newBest = (n.rssiSamples == 0) || (r > n.rssiMax);
        if (n.rssiSamples == 0) {
            n.rssiMin = r;
            n.rssiMax = r;
        } else {
            if (r < n.rssiMin) n.rssiMin = r;
            if (r > n.rssiMax) n.rssiMax = r;
        }
        n.rssi = r;
        n.snr  = snr;
        n.rssiSum += r;
        n.rssiSamples++;

        if (newBest && gps.valid) {
            n.hasPos = true;
            n.lat    = gps.lat;
            n.lon    = gps.lon;
        }

        // Frame-Counter: Lücken = Frames, die wir nicht gehört haben
        // (anderer Kanal, anderer SF oder zu weit weg)
        if (n.hasFcnt) {
            const int16_t d = (int16_t)(p.fcnt - n.lastFcnt);
            p.hasDelta = true;
            p.delta    = d;
            if (d > 1 && d <= 1000) {
                p.missed = d - 1;
                n.missedFrames += (uint32_t)p.missed;
            }
        }
        n.hasFcnt  = true;
        n.lastFcnt = p.fcnt;
    }

    cyclePackets++;
    totalPackets++;
    cycleDirect++;
    totalDirect++;

    // --- Log-Eintrag ---
    const String idStr = hex8(p.devAddr);
    const int    sid   = ScanContext::getOrAssignDeviceId(std::string(("LW" + idStr).c_str()));

    String fcntTxt = String(p.fcnt);
    if (p.hasDelta) {
        fcntTxt += " (" + String(p.delta >= 0 ? "+" : "") + String(p.delta);
        if (p.missed > 0) fcntTxt += ", missed " + String(p.missed);
        fcntTxt += ")";
    }

    String entry = "[#" + String(sid) + "] LW "
        + (isNew ? "NEW " : "")
        + idStr
        + ((p.hasDelta && p.delta == 0) ? "  [RETX]" : "")
        + "\n   [t=" + String(now / 1000.0f, 1) + "s]"
        + " RSSI " + String(rssi, 0) + " dBm | SNR " + String(snr, 1) + " dB"
        + " | " + String((unsigned)len) + " B | ToA " + String(toaUs / 1000) + " ms"
        + " | " + String(p.freqMhz, 1) + " MHz SF" + String(p.sf)
        + "\n   " + p.mtypeName
        + " | FCnt " + fcntTxt
        + " | FPort " + (p.hasPort ? String(p.fport) : String("-"))
        + " | payload " + String(p.payloadLen) + " B"
        + " | ADR " + String(p.adr ? 1 : 0)
        + " | ACK " + String(p.ack ? 1 : 0)
        + " | FOpts " + String(p.foptsLen);

    if (gps.valid) {
        entry += "\n   GPS " + String(gps.lat, 6) + ", " + String(gps.lon, 6)
               + " | alt " + String(gps.alt, 0) + " m | sat " + String(gps.sats);
    }
    entry += "\n   raw: " + toHex(buf, len);

    LOG(LOG_LORA, entry);
    csvAddLw(p, gps, now);

    // --- Neues Gerät: XP, Zähler, Nibbles mit Brille + DevAddr in der Sprechblase ---
    if (isNew) {
        ScanContext::allSpottedDevice++;
        DeviceContext::xpManager.awardXP(0.5f);

        displayName = "LW " + idStr;
        if (!UIContext::isAngryTaskRunning.load()) {
            UIExpressionTasks::showIfNotRunning(showGlassesExpressionTask, "LoraGlasses",
                UIContext::isGlassesTaskRunning, UIContext::glassesTaskHandle, 4096, 4, 1);
        }
    }
}

// ---------------------------------------------------------------------------
//  Ein empfangenes Paket lesen und je nach aktuellem Slot weiterreichen
// ---------------------------------------------------------------------------
static void handlePacket() {
    uint8_t buf[256];
    size_t  len = radio->getPacketLength();
    if (len > sizeof(buf)) len = sizeof(buf);

    int16_t st   = radio->readData(buf, len);
    float   rssi = radio->getRSSI();
    float   snr  = radio->getSNR();
    const uint32_t toaUs = (uint32_t)radio->getTimeOnAir(len);
    radio->startReceive();                       // sofort wieder lauschen

    const uint32_t now = millis();
    const GpsFix   gps = readGps();
    cycleAirUs += toaUs;                         // auch Fehlpakete belegen den Kanal

    // --- CRC-Fehler: Signal gehört, Inhalt kaputt ---
    if (st == RADIOLIB_ERR_CRC_MISMATCH) {
        cycleCrcErrors++;
        totalCrcErrors++;
        LOG(LOG_LORA, "[CRC ERROR] " + slotLabel() + " | " + String((unsigned)len) + " B | RSSI "
            + String(rssi, 0) + " dBm | SNR " + String(snr, 1) + " dB\n   raw: " + toHex(buf, len));
        csvAddError("CRC", rssi, snr, len, toaUs, buf, gps, now);
        return;
    }

    // --- Sonstiger Lesefehler ---
    if (st != RADIOLIB_ERR_NONE) {
        LOG(LOG_LORA, "[RX ERROR] " + slotLabel() + " | code " + String(st));
        return;
    }

    if (currentKind == Kind::LORAWAN) {
        handleLorawan(buf, len, rssi, snr, toaUs, now, gps);
    } else {
        handleMesh(buf, len, rssi, snr, toaUs, now, gps);
    }
}

// ---------------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------------
bool isSupported() { return true; }
bool isActive()    { return active; }

bool begin() {
    if (active) return true;

    if (!radio) {
        // Muss derselbe SPIClass sein, mit dem die SD-Karte initialisiert wurde
        radio = new SX1262(new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN,
                                      LORA_BUSY_PIN, SPI));
    }

    usedProfile = (RadioProfile)wantedProfile.load();
    buildSchedule(usedProfile);
    if (!configureSlot(schedule[0])) return false;
    nextHopAt = millis() + schedule[0].dwellMs;

    active = true;
    LOG(LOG_SYSTEM, String("LoRa scanner started, profile ") + profileName(usedProfile)
        + " (first slot " + slotLabel() + ")");
    return true;
}

void end() {
    if (!active || !radio) return;
    radio->clearPacketReceivedAction();
    radio->sleep();
    active = false;

    csvFlushAll();
    if (scannedSinceDump) {                      // nicht beim reinen Modul-Test (begin/end)
        dumpNodeTable();
        scannedSinceDump = false;
    }
    LOG(LOG_SYSTEM, "LoRa scanner stopped");
}

// ---------------------------------------------------------------------------
//  Ein Zyklus — Pendant zu scanForDevices()
//  Empfang läuft dauerhaft per Interrupt; der Zyklus verarbeitet nur
//  die eingegangenen Pakete, hüpft bei Bedarf auf den nächsten Slot
//  und schreibt die Summary.
// ---------------------------------------------------------------------------
void scan() {
    if (!active) {
        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    ScanContext::scanIsRunning.store(true);
    scannedSinceDump = true;
    applyPendingProfile();

    cyclePackets   = 0;
    cycleDirect    = 0;
    cycleRelayed   = 0;
    cycleDup       = 0;
    cycleCrcErrors = 0;
    cycleShort     = 0;
    cycleJoin      = 0;
    cycleOther     = 0;
    cycleAirUs     = 0;
    irqPolled      = 0;

    const uint32_t startMs = millis();
    const uint32_t endMs   = startMs + CYCLE_MS;
    uint32_t lastPoll = 0;

    while (millis() < endMs && !ScanContext::scanCancelRequested.load()) {
        // Diagnose/Fallback: RX_DONE direkt im Chip abfragen, falls der
        // DIO1-Interrupt nicht ankommt (falscher IRQ-Pin / Verdrahtung)
        if (!rxFlag && millis() - lastPoll >= 100) {
            lastPoll = millis();
            if (radio->getIrqFlags() & RADIOLIB_SX126X_IRQ_RX_DONE) {
                irqPolled++;
                rxFlag = true;
            }
        }
        if (rxFlag) {
            rxFlag = false;
            handlePacket();
        }

        // Erst nach dem Verarbeiten hüpfen, damit ein eben empfangenes Paket
        // noch mit der Konfiguration ausgewertet wird, mit der es ankam
        if (scheduleLen > 1 && millis() >= nextHopAt) {
            hopToNext();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    const uint32_t elapsed = millis() - startMs;
    const float airPct = elapsed ? (cycleAirUs / 1000.0f) * 100.0f / elapsed : 0.0f;

    String summary = String("LoRa summary [") + profileName(usedProfile) + "]: "
        + String(cyclePackets) + " pkts (" + String(cycleDirect) + " direct, "
        + String(cycleRelayed) + " relayed, " + String(cycleDup) + " dup), "
        + String(cycleCrcErrors) + " CRC err, " + String(cycleShort) + " short";
    if (usedProfile != RadioProfile::MESHTASTIC) {
        summary += ", " + String(cycleJoin) + " join, " + String(cycleOther) + " other";
    }
    summary += ", " + String((unsigned)nodeCount()) + " nodes"
        + " | airtime " + String(cycleAirUs / 1000) + " ms (" + String(airPct, 1) + "%)"
        + " | polled " + String(irqPolled)
        + " | RSSI now " + String(radio->getRSSI(false), 0) + " dBm"
        + " | slot " + slotLabel();

    LOG(LOG_SCAN, summary);
    if (cyclePackets > 0 || cycleCrcErrors > 0 || cycleShort > 0 || cycleOther > 0) {
        LOG(LOG_LORA, summary);                  // lora.log nur bei Aktivität
    }

    csvFlushAll();
    DeviceContext::xpManager.save();

    ScanContext::scanCancelRequested.store(false);
    ScanContext::scanIsRunning.store(false);
}

// ---------------------------------------------------------------------------
//  Profil-Steuerung (threadsicher, wird aus der UI aufgerufen)
// ---------------------------------------------------------------------------
RadioProfile getProfile() { return (RadioProfile)wantedProfile.load(); }

void setProfile(RadioProfile p) { wantedProfile.store((uint8_t)p); }

RadioProfile nextProfile() {
    RadioProfile n;
    switch (getProfile()) {
        case RadioProfile::AUTO:       n = RadioProfile::MESHTASTIC; break;
        case RadioProfile::MESHTASTIC: n = RadioProfile::LORAWAN;    break;
        default:                       n = RadioProfile::AUTO;       break;
    }
    setProfile(n);
    return n;
}

const char* profileName(RadioProfile p) {
    switch (p) {
        case RadioProfile::AUTO:       return "AUTO";
        case RadioProfile::MESHTASTIC: return "MESH";
        default:                       return "LORAWAN";
    }
}

char profileLetter() {
    switch (getProfile()) {
        case RadioProfile::MESHTASTIC: return 'M';
        case RadioProfile::LORAWAN:    return 'W';
        default:                       return 'L';
    }
}

// ---------------------------------------------------------------------------
//  Tracking, Zähler + Node-Liste (threadsicher, werden aus der UI aufgerufen)
// ---------------------------------------------------------------------------
void     setTrackedNode(uint32_t id) { trackedNode = id; }
uint32_t getTrackedNode()            { return trackedNode; }
uint32_t packetCount()               { return totalPackets; }
uint32_t directCount()               { return totalDirect; }
uint32_t crcErrorCount()             { return totalCrcErrors; }

size_t nodeCount() {
    std::lock_guard<std::mutex> lock(nodesMutex);
    return nodes.size();
}

void nodeCounts(size_t& mesh, size_t& lorawan) {
    mesh = 0;
    lorawan = 0;
    std::lock_guard<std::mutex> lock(nodesMutex);
    for (const auto& kv : nodes) {
        if (kv.second.lorawan) lorawan++;
        else                   mesh++;
    }
}

bool getNode(size_t index, Node& out) {
    std::lock_guard<std::mutex> lock(nodesMutex);
    if (index >= nodes.size()) return false;
    auto it = nodes.begin();
    std::advance(it, index);
    out = it->second;
    return true;
}

}  // namespace LoraScanner

#else  // kein LoRa-Cap auf diesem Board (z. B. StickS3)

namespace LoraScanner {
bool         isSupported()                 { return false; }
bool         begin()                       { return false; }
void         end()                         {}
bool         isActive()                    { return false; }
void         scan()                        {}
RadioProfile getProfile()                  { return RadioProfile::AUTO; }
void         setProfile(RadioProfile)      {}
RadioProfile nextProfile()                 { return RadioProfile::AUTO; }
const char*  profileName(RadioProfile)     { return "AUTO"; }
char         profileLetter()               { return 'L'; }
void         setTrackedNode(uint32_t)      {}
uint32_t     getTrackedNode()              { return 0; }
uint32_t     packetCount()                 { return 0; }
uint32_t     directCount()                 { return 0; }
uint32_t     crcErrorCount()               { return 0; }
size_t       nodeCount()                   { return 0; }
void         nodeCounts(size_t& m, size_t& l) { m = 0; l = 0; }
bool         getNode(size_t, Node&)        { return false; }
}

#endif
