#include "phoneAlertStatusService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "logger/logger.h"

String PhoneAlertStatusServiceHandler::readPhoneAlertStatus(NimBLEClient* pClient) {
    String result = "";

    if (!pClient) return result;

    NimBLERemoteService* service = pClient->getService("180E");
    if (!service) return result;

    LOG(LOG_GATT, "     Phone Alert Status Service detected (0x180E)");

    // Alert Status Characteristic (0x2A3F)
    NimBLERemoteCharacteristic* pStatus = service->getCharacteristic("2A3F");
    if (pStatus && pStatus->canRead()) {
        std::string raw = pStatus->readValue();
        if (!raw.empty()) {
            uint8_t status = raw[0];
            result = "Phone Alert: ";
            if (status & 0x01) result += "Ringer ";
            if (status & 0x02) result += "Vibrate ";
            if (status & 0x04) result += "Display ";
            if (status == 0) result += "Silent";
            //result += "\n";
            LOG(LOG_GATT, "     " + result);
        }
    }

    // Ringer Setting Characteristic (0x2A41)
    NimBLERemoteCharacteristic* pRinger = service->getCharacteristic("2A41");
    if (pRinger && pRinger->canRead()) {
        std::string raw = pRinger->readValue();
        if (!raw.empty()) {
            const char* setting = (raw[0] == 0) ? "Silent" : "Normal";
            result += "Ringer Setting: " + String(setting) + "\n";
            LOG(LOG_GATT, "     Ringer Setting: " + String(setting));
        }
    }

    if (result.isEmpty()) {
        result = "Phone Alert Status: Present\n";
    }

    return result;
}
