#include "hidService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "logger/logger.h"

String HIDServiceHandler::readHID(NimBLEClient* pClient) {
    String result = "";

    if (!pClient) return result;

    NimBLERemoteService* service = pClient->getService("1812");
    if (!service) return result;

    LOG(LOG_GATT, "     HID Service detected (0x1812)");

    result = "HID: Present";

    // HID Information Characteristic (0x2A4A)
    NimBLERemoteCharacteristic* pInfo = service->getCharacteristic("2A4A");
    if (pInfo && pInfo->canRead()) {
        std::string raw = pInfo->readValue();
        if (raw.size() >= 4) {
            uint16_t hidVersion = (uint8_t)raw[0] | ((uint8_t)raw[1] << 8);
            uint8_t countryCode = raw[2];
            uint8_t flags = raw[3];
            char buf[48];
            snprintf(buf, sizeof(buf), " (v%d.%d, country=%d, flags=0x%02X)",
                     hidVersion >> 8, hidVersion & 0xFF, countryCode, flags);
            result += String(buf);
            LOG(LOG_GATT, "     HID Info:" + String(buf));
        }
    }

    // Protocol Mode Characteristic (0x2A4E)
    NimBLERemoteCharacteristic* pMode = service->getCharacteristic("2A4E");
    if (pMode && pMode->canRead()) {
        std::string raw = pMode->readValue();
        if (!raw.empty()) {
            const char* mode = (raw[0] == 0) ? "Boot" : "Report";
            result += ", Mode: " + String(mode);
            LOG(LOG_GATT, "     Protocol Mode: " + String(mode));
        }
    }

    //result += "\n";
    return result;
}
