#include "device_context.h"

namespace DeviceContext {

// ------------------------------------------------------------
// Device configuration
// begin() must be called first in setup() —
// before NimBLE, WiFi, and anything else that needs a name.
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
