#pragma once

#include <Arduino.h>
#include <atomic>

#include "app/gamification/xp_manager.h"
#include "config/device_config.h"

// ============================================================
//  DeviceContext — Gerätekonfiguration, XP-System,
//                  Gamification, Target-Tracking
//
//  Thread-safety:
//    - DeviceConfig / XPManager  → nur aus loop() / setup(),
//                                  kein Task-Zugriff
//    - plain Felder              → nur aus loop() / setup()
// ============================================================

namespace DeviceContext {

// ------------------------------------------------------------
//  Gerätekonfiguration (NVS/Preferences)
//  Einzige Instanz im Projekt — deviceConfig.begin() in setup()
// ------------------------------------------------------------
extern DeviceConfig deviceConfig;

// ------------------------------------------------------------
//  XP-System / Gamification
//  Einzige Instanz im Projekt — xpManager.begin() in setup()
// ------------------------------------------------------------
extern XPManager xpManager;

// ------------------------------------------------------------
//  Target-Tracking
// ------------------------------------------------------------
extern std::atomic<int> targetConnects;  // Erfolgreiche GATT-Verbindungen
extern std::atomic<int> pointer;         // Manuell gesetzter Marker-Zähler

// ------------------------------------------------------------
//  Beacon-Zähler
//  Hier eingeordnet weil Beacons zum "was haben wir gefunden"
//  gehören — unabhängig vom laufenden Scan.
// ------------------------------------------------------------
extern std::atomic<int> beaconsFound;
extern std::atomic<int> pwnbeaconsFound;

} // namespace DeviceContext
