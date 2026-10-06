#include "lora_scanner.h"

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

// 1 = zusätzlich jedes Paket in /GhostBLE/lora_packets.csv schreiben, 0 = aus
#ifndef LORA_CSV_LOG
  #define LORA_CSV_LOG 1
#endif

namespace LoraScanner {

// Meshtastic EU868 LongFast — Werte bitte gegen die aktuelle Doku prüfen
static constexpr float    LORA_FREQ_MHZ  = 869.525f;
static constexpr float    LORA_BW_KHZ    = 250.0f;
static constexpr uint8_t  LORA_SF        = 11;
static constexpr uint8_t  LORA_CR        = 5;       // 4/5
static constexpr uint8_t  LORA_SYNC_WORD = 0x2B;
static constexpr uint16_t LORA_PREAMBLE  = 16;
static constexpr int8_t   LORA_POWER_DBM = 10;      // nur relevant fürs Senden

static constexpr uint32_t CYCLE_MS        = 5000;   // Verarbeitungszyklus, kein Scanfenster!
static constexpr size_t   MAX_NODES       = 64;
static constexpr size_t   MESH_HEADER     = 16;     // Klartext-Header eines Meshtastic-Pakets
static constexpr size_t   DEDUP_RING      = 64;     // gemerkte (from, packetId)-Paare
static constexpr uint32_t BROADCAST_ADDR  = 0xFFFFFFFF;
static constexpr size_t   CSV_FLUSH_BYTES = 6000;

static const char* const CSV_PATH   = "/GhostBLE/lora_packets.csv";
static const char* const CSV_HEADER =
    "ms,gps_time,type,from,to,packet_id,rssi_dbm,snr_db,len,toa_us,"
    "hop_limit,hop_start,hops_used,want_ack,via_mqtt,channel,next_hop,relay,dup,"
    "lat,lon,alt,sats,raw";

static SX1262*                  radio = nullptr;
static volatile bool            rxFlag = false;
static bool                     active = false;
static bool                     scannedSinceDump = false;
static uint32_t                 trackedNode = 0;

// nodes wird vom scanTask geschrieben und von der UI (Liste, Stats) gelesen
static std::map<uint32_t, Node> nodes;
static std::mutex               nodesMutex;

// Zähler pro Zyklus
static uint32_t cyclePackets   = 0;
static uint32_t cycleDirect    = 0;
static uint32_t cycleRelayed   = 0;
static uint32_t cycleDup       = 0;
static uint32_t cycleCrcErrors = 0;
static uint32_t cycleShort     = 0;
static uint32_t cycleAirUs     = 0;
static uint32_t irqPolled      = 0;   // Diagnose: RX_DONE per Polling statt DIO1-Interrupt

// Zähler seit Boot
static uint32_t totalPackets   = 0;
static uint32_t totalDirect    = 0;
static uint32_t totalCrcErrors = 0;

static String csvBuffer;

// Duplikaterkennung: dasselbe Paket kommt über Relays mehrfach an
struct SeenPkt { uint32_t from; uint32_t id; };
static SeenPkt seenRing[DEDUP_RING] = {};
static size_t  seenPos = 0;

// Alles, was wir über ein empfangenes Paket wissen
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

static String hex32(uint32_t v) {
    char b[12];
    snprintf(b, sizeof(b), "0x%08x", (unsigned)v);
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
//  CSV-Log (gepuffert, wird pro Zyklus in einem Rutsch geschrieben)
// ---------------------------------------------------------------------------
static void csvFlush() {
#if LORA_CSV_LOG
    if (csvBuffer.isEmpty()) return;

    const bool isNewFile = !SD.exists(CSV_PATH);
    File f = SD.open(CSV_PATH, FILE_APPEND);
    if (f) {
        if (isNewFile) f.println(CSV_HEADER);
        f.print(csvBuffer);
        f.close();
    }
    csvBuffer.clear();   // auch bei Fehler leeren, damit der Heap nicht wächst
#endif
}

static void csvAdd(const PacketInfo& p, const GpsFix& g, uint32_t now) {
#if LORA_CSV_LOG
    auto col  = [](String& line, const String& v) { line += v; line += ','; };
    auto opt  = [&](bool has, const String& v) { return has ? v : String(""); };

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

    csvBuffer += l;
    if (csvBuffer.length() > CSV_FLUSH_BYTES) csvFlush();
#endif
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

            out += "\n   " + nodeIdToString(n.id)
                 + " | pkts " + String(n.packets)
                 + " (direct " + String(n.direct) + " / relayed " + String(n.relayed) + ")";

            if (n.rssiSamples > 0) {
                out += " | RSSI " + String(n.rssi)
                     + " [" + String(n.rssiMin) + ".." + String(n.rssiMax) + "]"
                     + " avg " + String(n.rssiSum / (int32_t)n.rssiSamples)
                     + " | SNR " + String(n.snr, 1);
            } else {
                out += " | RSSI n/a (nur ueber Relays gehoert)";
            }

            out += " | ch 0x" + hex2(n.channel)
                 + " | first " + String(n.firstSeen / 1000) + "s"
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
//  Ein empfangenes Paket verarbeiten
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

    PacketInfo p;
    p.rssi  = rssi;
    p.snr   = snr;
    p.len   = len;
    p.toaUs = toaUs;
    p.raw   = buf;
    cycleAirUs += toaUs;                         // auch Fehlpakete belegen den Kanal

    // --- CRC-Fehler: Signal gehört, Inhalt kaputt ---
    if (st == RADIOLIB_ERR_CRC_MISMATCH) {
        cycleCrcErrors++;
        totalCrcErrors++;
        p.type = "CRC";
        LOG(LOG_LORA, "[CRC ERROR] " + String((unsigned)len) + " B | RSSI " + String(rssi, 0)
            + " dBm | SNR " + String(snr, 1) + " dB\n   raw: " + toHex(buf, len));
        csvAdd(p, gps, now);
        return;
    }

    // --- Sonstiger Lesefehler ---
    if (st != RADIOLIB_ERR_NONE) {
        p.type = "ERR";
        LOG(LOG_LORA, "[RX ERROR] code " + String(st));
        return;
    }

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

        auto it = nodes.find(p.from);
        isNew = (it == nodes.end());
        if (isNew) {
            if (nodes.size() >= MAX_NODES) evictOldest();
            it = nodes.emplace(p.from, Node{}).first;
            it->second.id        = p.from;
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

    int16_t st = radio->begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                              LORA_SYNC_WORD, LORA_POWER_DBM, LORA_PREAMBLE);
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

    rxFlag = false;
    active = true;
    LOG(LOG_SYSTEM, "LoRa scanner started (" + String(LORA_FREQ_MHZ, 3) + " MHz, SF"
        + String(LORA_SF) + ", BW" + String(LORA_BW_KHZ, 0) + ")");
    return true;
}

void end() {
    if (!active || !radio) return;
    radio->clearPacketReceivedAction();
    radio->sleep();
    active = false;

    csvFlush();
    if (scannedSinceDump) {                      // nicht beim reinen Modul-Test (begin/end)
        dumpNodeTable();
        scannedSinceDump = false;
    }
    LOG(LOG_SYSTEM, "LoRa scanner stopped");
}

// ---------------------------------------------------------------------------
//  Ein Zyklus — Pendant zu scanForDevices()
//  Empfang läuft dauerhaft per Interrupt; der Zyklus verarbeitet nur
//  die eingegangenen Pakete und schreibt die Summary.
// ---------------------------------------------------------------------------
void scan() {
    if (!active) {
        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    ScanContext::scanIsRunning.store(true);
    scannedSinceDump = true;
    cyclePackets   = 0;
    cycleDirect    = 0;
    cycleRelayed   = 0;
    cycleDup       = 0;
    cycleCrcErrors = 0;
    cycleShort     = 0;
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
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    const uint32_t elapsed = millis() - startMs;
    const float airPct = elapsed ? (cycleAirUs / 1000.0f) * 100.0f / elapsed : 0.0f;

    const String summary = "LoRa summary: " + String(cyclePackets) + " pkts ("
        + String(cycleDirect) + " direct, " + String(cycleRelayed) + " relayed, "
        + String(cycleDup) + " dup), " + String(cycleCrcErrors) + " CRC err, "
        + String(cycleShort) + " short, " + String((unsigned)nodeCount()) + " nodes"
        + " | airtime " + String(cycleAirUs / 1000) + " ms (" + String(airPct, 1) + "%)"
        + " | polled " + String(irqPolled)
        + " | RSSI now " + String(radio->getRSSI(false), 0) + " dBm";

    LOG(LOG_SCAN, summary);
    if (cyclePackets > 0 || cycleCrcErrors > 0 || cycleShort > 0) {
        LOG(LOG_LORA, summary);                  // lora.log nur bei Aktivität
    }

    csvFlush();
    DeviceContext::xpManager.save();

    ScanContext::scanCancelRequested.store(false);
    ScanContext::scanIsRunning.store(false);
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
bool     isSupported()               { return false; }
bool     begin()                     { return false; }
void     end()                       {}
bool     isActive()                  { return false; }
void     scan()                      {}
void     setTrackedNode(uint32_t)    {}
uint32_t getTrackedNode()            { return 0; }
uint32_t packetCount()               { return 0; }
uint32_t directCount()               { return 0; }
uint32_t crcErrorCount()             { return 0; }
size_t   nodeCount()                 { return 0; }
bool     getNode(size_t, Node&)      { return false; }
}

#endif
