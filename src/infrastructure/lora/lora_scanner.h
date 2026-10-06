#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
//  LoRa-Scanner — Alternative zum BLE-Scanner (nur Cardputer mit LoRa-Cap)
//  Passiver Empfang: gesehen werden Nodes, deren Pakete zur eingestellten
//  Funk-Konfiguration passen (hier: Meshtastic EU868 LongFast).
// ---------------------------------------------------------------------------
namespace LoraScanner {

struct Node {
    uint32_t id          = 0;      // Meshtastic Node-ID (Header-Feld "from")

    // RSSI/SNR nur aus direkt gehörten Paketen (hops == 0 oder unbekannt).
    // Bei weitergeleiteten Paketen misst man das Relay, nicht den Absender.
    int16_t  rssi        = 0;      // letzter Wert
    int16_t  rssiMin     = 0;
    int16_t  rssiMax     = 0;
    int32_t  rssiSum     = 0;
    uint32_t rssiSamples = 0;
    float    snr         = 0;      // letzter Wert

    uint32_t packets     = 0;      // alle gehörten Pakete (inkl. Relay/Duplikate)
    uint32_t direct      = 0;      // davon direkt (hops == 0)
    uint32_t relayed     = 0;      // davon über Relays (hops > 0)
    uint8_t  lastHops    = 0xFF;   // 0xFF = unbekannt (ältere Firmware)
    uint8_t  channel     = 0;      // Channel-Hash des letzten Pakets

    bool     hasPos      = false;  // Position beim bisher stärksten Direkt-Empfang
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

// Pendant zur Connected-Device-Liste: Pakete dieses Nodes werden im Log markiert
void     setTrackedNode(uint32_t id);
uint32_t getTrackedNode();

// Zähler seit Boot
uint32_t packetCount();       // gültige Pakete
uint32_t directCount();       // davon direkt gehört (hops == 0)
uint32_t crcErrorCount();     // Pakete mit CRC-Fehler

// Für die Menü-Liste
size_t nodeCount();
bool   getNode(size_t index, Node& out);   // für Menü-Liste

}  // namespace LoraScanner
