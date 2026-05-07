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

static const std::map<uint16_t, SdoEntry> sdoTable = {
    {0xFFFA, SdoEntry{"ASTM Remote ID (Drone)", SDO_CAT_DRONE, SDO_THREAT_HIGH, std::bind(SdoHandlers::handleDrone, nullptr)}},
    {0xFFFB, SdoEntry{"Thread Network", SDO_CAT_IOT, SDO_THREAT_LOW, nullptr}},
    {0xFFFC, SdoEntry{"AirFuel Charging", SDO_CAT_MISC, SDO_THREAT_LOW, nullptr}},
    {0xFFFD, SdoEntry{"FIDO U2F", SDO_CAT_SECURITY, SDO_THREAT_LOW, std::bind(SdoHandlers::handleFido, nullptr)}},
    {0xFFF6, SdoEntry{"Matter Device", SDO_CAT_IOT, SDO_THREAT_MEDIUM, std::bind(SdoHandlers::handleMatter, nullptr)}},
    {0xFFF7, SdoEntry{"Zigbee Direct", SDO_CAT_IOT, SDO_THREAT_MEDIUM, nullptr}},
};

// ===== Core Parser =====

bool SdoServiceParser::parse(uint16_t uuid, SdoResult& result) {

    auto it = sdoTable.find(uuid);
    if (it == sdoTable.end()) {
        return false;
    }

    const SdoEntry& entry = it->second;

    result.uuid = uuid;
    result.name = entry.name;
    result.category = entry.category;
    result.threat = entry.threat;

    // Logging
    LOG(LOG_GATT, "     [SDO] Detected: " + String(entry.name));

    // Execute optional handler
    if (entry.handler) {
        entry.handler();
    }

    return true;
}
