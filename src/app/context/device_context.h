#pragma once

#include <Arduino.h>
#include <atomic>

#include "app/gamification/xp_manager.h"
#include "config/device_config.h"

// ============================================================
//  DeviceContext — device configuration, XP system,
//                  gamification, target tracking
//
//  Thread-safety:
//    - DeviceConfig / XPManager  → accessed from loop() / setup()
//                                  only, no FreeRTOS task access
//    - plain fields              → loop() / setup() only
// ============================================================

namespace DeviceContext {

// ------------------------------------------------------------
//  Device configuration (NVS/Preferences)
//  Single instance — call deviceConfig.begin() first in setup()
// ------------------------------------------------------------
extern DeviceConfig deviceConfig;

// ------------------------------------------------------------
//  XP system / gamification
//  Single instance — call xpManager.begin() after initLogger()
// ------------------------------------------------------------
extern XPManager xpManager;

// ------------------------------------------------------------
//  Target tracking
// ------------------------------------------------------------
extern std::atomic<int> targetConnects;  // successful GATT connections
extern std::atomic<int> pointer;         // manually placed marker count

// ------------------------------------------------------------
//  Beacon counters
//  Kept here because beacons are a discovery result independent
//  of the active scan cycle.
// ------------------------------------------------------------
extern std::atomic<int> beaconsFound;
extern std::atomic<int> pwnbeaconsFound;

} // namespace DeviceContext