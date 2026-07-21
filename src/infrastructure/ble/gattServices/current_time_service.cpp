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

    // Fallback: Manche Geräte (z.B. Xiaomi Wearables) exponieren 2A2B
    // unter einem proprietären Service statt 0x1805 — alle Services durchsuchen
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

    if (!pChar || !pChar->canRead()) {
        return timeStr;
    }

    std::string raw = pChar->readValue();
    if (raw.size() < 7) {
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

    NimBLERemoteCharacteristic* pDow = nullptr;
    if (timeService) {
        pDow = timeService->getCharacteristic("2A09");
    }
    if (!pDow) {
        // Auch Day-of-Week ggf. im selben proprietären Service suchen
        for (auto& svc : pClient->getServices()) {
            NimBLERemoteCharacteristic* candidate = svc->getCharacteristic("2A09");
            if (candidate) {
                pDow = candidate;
                break;
            }
        }
    }

    if (pDow && pDow->canRead()) {
        std::string dowRaw = pDow->readValue();
        if (!dowRaw.empty()) {
            const char* days[] = {"", "Monday", "Tuesday", "Wednesday",
                                  "Thursday", "Friday", "Saturday", "Sunday"};
            uint8_t dow = dowRaw[0];
            if (dow >= 1 && dow <= 7) {
                timeStr += "Day of Week: " + String(days[dow]) + "\n";
                LOG(LOG_SCAN, "     Day of Week: " + String(days[dow]));
            }
        }
    }

    return timeStr;
}
