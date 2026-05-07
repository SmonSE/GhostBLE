#include "sdo_service_parser.h"

#include "infrastructure/logging/logger.h"
#include "infrastructure/ble/handler/sdo_handlers.h"

#include <map>
#include <functional>

// ===== SDO Definition =====

struct SdoEntry {
    const char* name;
    SdoCategory category;
    SdoThreatLevel threat;
    SdoHandler handler;
};

// ===== Forward Handler =====
static void handleDrone();
static void handleFido();
static void handleMatter();

// ===== SDO Table =====
// Statt std::map — statisches Array, kein Heap:
static const struct {
    uint16_t    uuid;
    const char* name;
    SdoCategory category;
    SdoThreatLevel threat;
    void (*handler)(const SdoContext*);
} sdoTable[] = {
    { 0xFFF6, "Matter Device",           SDO_CAT_IOT,      SDO_THREAT_MEDIUM, SdoHandlers::handleMatter },
    { 0xFFF7, "Zigbee Direct",           SDO_CAT_IOT,      SDO_THREAT_MEDIUM, nullptr                  },
    { 0xFFFA, "ASTM Remote ID (Drone!)", SDO_CAT_DRONE,    SDO_THREAT_HIGH,   SdoHandlers::handleDrone  },
    { 0xFFFB, "Thread Network",          SDO_CAT_IOT,      SDO_THREAT_LOW,    nullptr                   },
    { 0xFFFC, "AirFuel Wireless Charging", SDO_CAT_MISC,   SDO_THREAT_LOW,    nullptr                   },
    { 0xFFFD, "FIDO U2F",               SDO_CAT_SECURITY, SDO_THREAT_LOW,    SdoHandlers::handleFido   },
};
static constexpr size_t SDO_TABLE_SIZE = sizeof(sdoTable) / sizeof(sdoTable[0]);

// ===== Core Parser =====
bool SdoServiceParser::parse(uint16_t uuid, SdoResult& result) {

    for (size_t i = 0; i < SDO_TABLE_SIZE; ++i) {
        if (sdoTable[i].uuid == uuid) {
            const auto& entry = sdoTable[i];

            result.uuid = uuid;
            result.name = entry.name;
            result.category = entry.category;
            result.threat = entry.threat;

            // Logging
            LOG(LOG_GATT, "     [SDO] Detected: " + String(entry.name));

            // Execute optional handler
            if (entry.handler) {
                SdoContext ctx;
                ctx.rssi = 0;   // oder aus ScanContext
                ctx.mac  = "";
                entry.handler(&ctx);
            }

            return true;
        }
    }

    return false;
}

