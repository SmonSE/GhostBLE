#include "sdo_handlers.h"
#include <M5Unified.h>

#include "infrastructure/logging/logger.h"
#include "core/parsing/drone_id_parser.h"
#include "app/context/device_context.h"
#include "app/context/scan_context.h"

#include "ui/menu/menu_controller.h"

static unsigned long lastDroneAlert = 0;
static const unsigned long DRONE_ALERT_COOLDOWN = 5000;

void SdoHandlers::handleDrone(const SdoContext* ctx) {
    unsigned long now = millis();
    if (now - lastDroneAlert < DRONE_ALERT_COOLDOWN) return;
    lastDroneAlert = now;

    // Parse Remote ID payload if available
    if (ctx && ctx->serviceData && ctx->serviceDataLen > 0) {
        DroneIDResult drone = parseDroneID(ctx->serviceData, ctx->serviceDataLen);

        if (drone.valid) {
            LOG(LOG_BEACON, "[SDO] " + drone.summary());
            DeviceContext::xpManager.awardXP(5.0f);  // +5.0 XP: Drone ID decoded

            // ← Audio alert
            auto* ms = MenuController::getState();
            if (ms->audioEnabled && ms->audioDrone) {
                M5.Speaker.setVolume(MenuController::getAlarmVolume());
                M5.Speaker.tone(2093, 150);  // hoher Ton = Drohne
                delay(200);
                M5.Speaker.tone(1568, 150);
                delay(200);
                M5.Speaker.tone(2093, 150);
            }

            ScanContext::susDevice++;
            if (drone.isEmergency()) {
                LOG(LOG_TARGET, "[SDO] !!! DRONE EMERGENCY STATUS !!!");
            }

            if (drone.hasOperator()) {
                LOG(LOG_GPS, "[SDO] Pilot location: "
                    + String(drone.system.operatorLat, 6) + ", "
                    + String(drone.system.operatorLon, 6));
            }
            return;
        }
    }

    // Fallback: no payload or unparseable
    String msg = "[SDO] Drone detected (ASTM Remote ID)";
    if (ctx) msg += " | RSSI: " + String(ctx->rssi) + " | MAC: " + ctx->mac;
    LOG(LOG_GATT, msg);
}

// ===== FIDO Handler =====
void SdoHandlers::handleFido(const SdoContext* ctx) {

    String msg = "     [SDO] FIDO Security Device detected";

    if (ctx) {
        msg += " | RSSI: " + String(ctx->rssi);
    }

    LOG(LOG_GATT, msg);

    // Optional:
    // UI::showSecurityIcon();
}

// ===== Matter Handler =====
void SdoHandlers::handleMatter(const SdoContext* ctx) {

    String msg = "     [SDO] Matter IoT device detected";

    if (ctx) {
        msg += " | RSSI: " + String(ctx->rssi);
    }

    LOG(LOG_GATT, msg);

    // Optional:
    // UI::showIoTIcon();
}

bool SdoHandlers::extract16BitUUID(const NimBLEUUID& uuid, uint16_t& out) {
    std::string s = uuid.toString();  // z.B. "0000fffa-0000-1000-8000-00805f9b34fb"

    // Fall 1: kurze Form "fffa"
    if (s.length() == 4) {
        out = (uint16_t) strtol(s.c_str(), nullptr, 16);
        return true;
    }

    // Fall 2: lange 128-bit Form → Bluetooth Base UUID
    if (s.length() == 36) {
        // Format: 0000XXXX-0000-1000-8000-00805f9b34fb
        std::string shortPart = s.substr(4, 4);
        out = (uint16_t) strtol(shortPart.c_str(), nullptr, 16);
        return true;
    }

    return false;
}
