#include "current_time_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"

String CurrentTimeServiceHandler::readCurrentTime(NimBLEClient* pClient) {
    String timeStr = "";
    if (!pClient) return timeStr;

    NimBLERemoteCharacteristic* pChar = nullptr;

    // Erst: Standard-Weg über Service 0x1805 versuchen
    NimBLERemoteService* timeService = pClient->getService("1805");
    if (timeService) {
        pChar = timeService->getCharacteristic("2A2B");
        if (pChar) {
            LOG(LOG_SCAN, "     Current Time Service detected (0x1805)");
        }
    }

    // Fallback: Some devices (e.g., Xiaomi wearables) expose 2A2B
    // under a proprietary service instead of 0x1805 — search all services
    if (!pChar) {
        for (auto& svc : pClient->getServices()) {
            NimBLERemoteCharacteristic* candidate = svc->getCharacteristic("2A2B");
            if (candidate) {
                pChar = candidate;
                LOG(LOG_SCAN, "     Current Time char found under non-standard service");
                break;
            }
        }
    }

    if (!pChar) {
        LOG(LOG_SCAN, "     No 2A2B (Current Time) characteristic found on this device");
        return timeStr;
    }

    if (!pChar->canRead()) {
        String propStr = "";
        if (pChar->canWrite())          propStr += "write ";
        if (pChar->canWriteNoResponse()) propStr += "writeNoResp ";
        if (pChar->canNotify())         propStr += "notify ";
        if (pChar->canIndicate())       propStr += "indicate ";
        if (pChar->canBroadcast())      propStr += "broadcast ";
        if (propStr.isEmpty())          propStr = "none";
        LOG(LOG_SCAN, "     2A2B found but not readable (props: " + propStr + ")");
        return timeStr;
    }

    std::string raw = pChar->readValue();
    if (raw.empty()) {
        LOG(LOG_SCAN, "     2A2B read returned empty (encryption/bonding required?)");
        return timeStr;
    }
    if (raw.size() < 7) {
        LOG(LOG_SCAN, "     2A2B read too short: " + String((int)raw.size()) + " bytes");
        return timeStr;
    }

    uint16_t year = (uint8_t)raw[0] | ((uint8_t)raw[1] << 8);
    uint8_t month = raw[2];
    uint8_t day = raw[3];
    uint8_t hours = raw[4];
    uint8_t minutes = raw[5];
    uint8_t seconds = raw[6];

    char timeBuf[32];
    snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hours, minutes, seconds);

    timeStr = "Device Time: " + String(timeBuf) + "\n";
    LOG(LOG_SCAN, "     Device Time: " + String(timeBuf));

    // Siehe Bluetooth Core Spec: Current Time Service / Day Date Time.
    if (raw.size() >= 8) {
        uint8_t dow = (uint8_t)raw[7];
        const char* days[] = {"", "Monday", "Tuesday", "Wednesday",
                              "Thursday", "Friday", "Saturday", "Sunday"};
        if (dow >= 1 && dow <= 7) {
            timeStr += "Day of Week: " + String(days[dow]) + "\n";
            LOG(LOG_SCAN, "     Day of Week: " + String(days[dow]));
        }
    }

    return timeStr;
}
