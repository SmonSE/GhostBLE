#pragma once
#include <Arduino.h>

#include "ui/expression/show_expression.h"

// ---------------------------------------------------------------------------
//  LoRa-Scanner — Alternative zum BLE-Scanner (nur Cardputer mit LoRa-Cap)
//  Passiver Empfang. Das Radio hört immer genau EINE Konfiguration
//  (Frequenz + SF + Sync Word). Welche, bestimmt das Profil:
//    AUTO        wechselt selbstständig zwischen Meshtastic und LoRaWAN
//    MESHTASTIC  nur Meshtastic EU868 LongFast
//    LORAWAN     nur LoRaWAN-Uplinks, hüpfend über alle 8 TTN-Kanäle
//                (868.1/.3/.5 und 867.1-867.9 MHz) = Survey-Profil
// ---------------------------------------------------------------------------
namespace LoraScanner {

enum class RadioProfile : uint8_t { AUTO = 0, MESHTASTIC = 1, LORAWAN = 2 };

struct Node {
    uint32_t id          = 0;      // Meshtastic Node-ID oder LoRaWAN DevAddr
    bool     lorawan     = false;  // false = Meshtastic, true = LoRaWAN

    // RSSI/SNR nur aus direkt gehörten Paketen (bei Meshtastic-Relays misst man das Relay).
    int16_t  rssi        = 0;      // letzter Wert
    int16_t  rssiMin     = 0;
    int16_t  rssiMax     = 0;
    int32_t  rssiSum     = 0;
    uint32_t rssiSamples = 0;
    float    snr         = 0;      // letzter Wert

    uint32_t packets     = 0;      // alle gehörten Pakete
    uint32_t direct      = 0;      // davon direkt (Mesh: hops == 0, LoRaWAN: immer)
    uint32_t relayed     = 0;      // nur Mesh: über Relays
    uint8_t  lastHops    = 0xFF;   // nur Mesh, 0xFF = unbekannt
    uint8_t  channel     = 0;      // nur Mesh: Channel-Hash des letzten Pakets

    // nur LoRaWAN
    bool     hasFcnt      = false;
    uint16_t lastFcnt     = 0;
    uint32_t missedFrames = 0;     // aus Lücken im Frame-Counter geschätzt
    uint8_t  lastFport    = 0;
    float    freqMhz      = 0;     // zuletzt gehörte Frequenz
    uint8_t  sf           = 0;     // zuletzt gehörter Spreading Factor

    bool     hasPos      = false;  // Position beim bisher stärksten Empfang
    double   lat         = 0;
    double   lon         = 0;

    uint32_t firstSeen   = 0;      // millis()
    uint32_t lastSeen    = 0;      // millis()
};

bool isSupported();           // Compile-Time: Board hat LoRa-Cap (LORA_CS_PIN)
bool begin();                 // false = Modul nicht gefunden / Init fehlgeschlagen
void end();                   // Radio in Sleep (schreibt vorher CSV-Rest + Node-Tabelle)
bool isActive();

void scan();                  // ein Zyklus, Ersatz für scanForDevices() im LoRa-Modus

// Profil (wirkt spätestens im nächsten Zyklus, auch bei laufendem Scan)
RadioProfile getProfile();
void         setProfile(RadioProfile p);
RadioProfile nextProfile();               // AUTO -> MESHTASTIC -> LORAWAN -> AUTO
const char*  profileName(RadioProfile p); // "AUTO", "MESH", "LORAWAN"
char         profileLetter();             // 'L' (auto), 'M' (Meshtastic), 'W' (LoRaWAN)

// Pakete dieses Meshtastic-Nodes werden im Log markiert
void     setTrackedNode(uint32_t id);
uint32_t getTrackedNode();

// Zähler seit Boot
uint32_t packetCount();       // gültige Pakete
uint32_t directCount();       // davon direkt gehört
uint32_t crcErrorCount();     // Pakete mit CRC-Fehler

// Für die Menü-Liste
size_t nodeCount();
void   nodeCounts(size_t& mesh, size_t& lorawan);   // Nodes je Protokoll (mesh + lorawan == nodeCount())
bool   getNode(size_t index, Node& out);

}  // namespace LoraScanner
