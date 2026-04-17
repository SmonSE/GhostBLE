#include "current_time_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"

String CurrentTimeServiceHandler::readCurrentTime(NimBLEClient* pClient) {
    String timeStr = "";

    if (!pClient) return timeStr;

    // Standard BLE Current Time Service (0x1805)
    NimBLERemoteService* timeService = pClient->getService("1805");
    if (!timeService) {
        return timeStr;
    }

    LOG(LOG_GATT,"     Current Time Service detected (0x1805)");

    // Current Time Characteristic (0x2A2B)
    NimBLERemoteCharacteristic* pChar = timeService->getCharacteristic("2A2B");
    if (!pChar || !pChar->canRead()) {
        return timeStr;
    }

    std::string raw = pChar->readValue();
    if (raw.size() < 7) {
        return timeStr;
    }

    // Current Time format: year(2) month(1) day(1) hours(1) minutes(1) seconds(1)
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
    LOG(LOG_GATT,"     Device Time: " + String(timeBuf));

    // Day of Week (0x2A09) if available
    NimBLERemoteCharacteristic* pDow = timeService->getCharacteristic("2A09");
    if (pDow && pDow->canRead()) {
        std::string dowRaw = pDow->readValue();
        if (!dowRaw.empty()) {
            const char* days[] = {"", "Monday", "Tuesday", "Wednesday",
                                  "Thursday", "Friday", "Saturday", "Sunday"};
            uint8_t dow = dowRaw[0];
            if (dow >= 1 && dow <= 7) {
                timeStr += "Day of Week: " + String(days[dow]) + "\n";
                LOG(LOG_GATT,"     Day of Week: " + String(days[dow]));
            }
        }
    }

    return timeStr;
}
