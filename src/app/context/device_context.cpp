#include "device_context.h"

namespace DeviceContext {

// ------------------------------------------------------------
//  Gerätekonfiguration
//  begin() muss als erstes in setup() aufgerufen werden —
//  vor NimBLE, WiFi und allem was den Namen braucht.
// ------------------------------------------------------------
DeviceConfig deviceConfig;

// ------------------------------------------------------------
// XP system
// call begin() after initLogger() (requires SD card).
// ------------------------------------------------------------
XPManager xpManager;

// ------------------------------------------------------------
//  Target-Tracking
// ------------------------------------------------------------
std::atomic<int> targetConnects{0};
std::atomic<int> pointer{0};

// ------------------------------------------------------------
//  Beacon-Counter
// ------------------------------------------------------------
std::atomic<int> beaconsFound{0};
std::atomic<int> pwnbeaconsFound{0};

} // namespace DeviceContext
