#pragma once

#include <Arduino.h>
#include <atomic>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

// ============================================================
//  ScanContext — alles was den BLE-Scan-Lebenszyklus beschreibt
//
//  Thread-safety-Garantien:
//    - std::atomic<>  → sicher zwischen ScanTask (Core 1)
//                       und loop() (Core 0)
//    - plain Felder   → nur aus loop() / setup() zugänglich,
//                       kein FreeRTOS-Task schreibt darauf
// ============================================================

namespace ScanContext {

// ------------------------------------------------------------
//  Scan-Steuerung  (Core 0 schreibt, Core 1 liest → atomic)
// ------------------------------------------------------------
extern std::atomic<bool> bleScanEnabled;   // User hat Scan aktiviert
extern std::atomic<bool> scanIsRunning;    // Scan-Schleife läuft gerade

// ------------------------------------------------------------
//  Gefundene Geräte
// ------------------------------------------------------------
extern std::unordered_set<std::string> seenDevices;   // Dedup-Set (MAC-Strings)
extern std::map<std::string, int>      deviceSessionMap;
extern std::atomic<int>                nextDeviceSessionId;

// Weist einer MAC eine aufsteigende Session-ID zu (oder gibt
// die bestehende zurück). Thread-safe für den Scan-Task.
int getOrAssignDeviceId(const std::string& mac);

// Temporäre Listen für den aktuellen Scan-Durchlauf.
// Werden vom Scanner befüllt und nach Verarbeitung geleert.
extern std::vector<std::string> uuidList;
extern std::vector<std::string> nameList;

// ------------------------------------------------------------
//  Zähler  (Core 1 schreibt, Core 0 liest → atomic)
// ------------------------------------------------------------
extern std::atomic<int> allSpottedDevice;          // Gesamt entdeckt
extern std::atomic<int> susDevice;                 // Verdächtig eingestuft
extern std::atomic<int> leakedCounter;             // Cleartext-Leaks
extern std::atomic<int> rssi;                      // RSSI des letzten Geräts
extern std::atomic<int> beaconsFound;      
extern std::atomic<int> pwnbeaconsFound;

// ------------------------------------------------------------
//  Sicherheits-Scores  (reset pro Scan-Zyklus)
// ------------------------------------------------------------
extern std::atomic<int> riskScore;
extern std::atomic<int> highFindingsCount;
extern std::atomic<int> unencryptedSensitiveCount;
extern std::atomic<int> writableNoAuthCount;

// ------------------------------------------------------------
//  Target-Tracking
// ------------------------------------------------------------
extern bool isTarget;
extern bool targetFound;
extern std::atomic<int> targetConnects;

// ------------------------------------------------------------
//  Verbindungsstrings  (nur im loop() / Callbacks verwendet)
// ------------------------------------------------------------
extern std::string addrStr;          // Aktuelle Geräte-Adresse
extern String      is_connectable;   // "Yes" / "No"

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------

// Setzt alle Zähler und temporären Listen zurück.
// Aufrufen am Beginn jedes neuen Scan-Durchlaufs.
void reset();

// Räumt seenDevices + deviceSessionMap wenn der Heap knapp wird
// oder die Menge zu groß ist. Gibt true zurück wenn bereinigt.
bool reactiveCleanup(size_t maxDevices, uint32_t minFreeHeap);

} // namespace ScanContext
