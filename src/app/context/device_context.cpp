#include "device_context.h"

namespace DeviceContext {

// ------------------------------------------------------------
//  Gerätekonfiguration
//  begin() muss als erstes in setup() aufgerufen werden —
//  vor NimBLE, WiFi und allem was den Namen braucht.
// ------------------------------------------------------------
DeviceConfig deviceConfig;

// ------------------------------------------------------------
//  XP-System
//  begin() nach initLogger() aufrufen (braucht SD-Karte).
// ------------------------------------------------------------
XPManager xpManager;

// ------------------------------------------------------------
//  Target-Tracking
// ------------------------------------------------------------
std::atomic<int> targetConnects{0};
std::atomic<int> pointer{0};

// ------------------------------------------------------------
//  Beacon-Zähler
// ------------------------------------------------------------
std::atomic<int> beaconsFound{0};
std::atomic<int> pwnbeaconsFound{0};

} // namespace DeviceContext
