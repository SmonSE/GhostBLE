#include "genericAccessService.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

#include "../globals/globals.h"
#include "../config/config.h"
#include "../logger/logger.h"
#include "../helper/AppearanceHelper.h"


String GenericAccessServiceHandler::readGenericAccessInfo(NimBLEClient* pClient) {
    String accessInfoString = "";

    // Add the LOG(LOG_GATT, devTag + "Reading Generic Access Service (0x1800)");
    LOG(LOG_GATT, devTag + "Reading Generic Access Service (0x1800)");

    // For better logging and correlation, show local name if available, otherwise just the address
    if (localName != "") {
        LOG(LOG_GATT, "   Device found: " + localName + " [" + address + "]");
    } else {
        LOG(LOG_GATT, "   Device found: [" + address + "]");
    }

    NimBLERemoteService* gapService = pClient->getService(UUID_GENERIC_ACCESS);
    if (!gapService) {
        LOG(LOG_GATT,"   Generic Access Service not found (0x1800)");
        return accessInfoString;
    }

    LOG(LOG_GATT,"   Generic Access Service found (0x1800)");

    const char* charUUIDs[] = {"2A00", "2A01", "2A04", "2AA6"};
    const char* charNames[] = {
        "Device Name",
        "Appearance",
        "Preferred Connection Parameters",
        "Central Address Resolution"
    };

    LOG(LOG_GATT,"     Read value of generic access info");
    
    for (int i = 0; i < 4; i++) {
        NimBLERemoteCharacteristic* pChar = gapService->getCharacteristic(charUUIDs[i]);
        if (!pChar) {
            LOG(LOG_GATT, "     Characteristic " + String(charUUIDs[i]) + " not found.");
            continue;
        }

        if (pChar->canRead()) {
            std::string value = pChar->readValue();
            
            if (strcmp(charUUIDs[i], "2A01") == 0) {
                if (value.size() >= 2) {
                    uint16_t appearance;
                    memcpy(&appearance, value.data(), sizeof(appearance));
                    appearanceName = getAppearanceName(appearance);
                    accessInfoString += "Appearance: " + appearanceName + " (0x" + String(appearance, HEX) + ")\n";
                    LOG(LOG_GATT, "     Appearance: " + appearanceName + " (0x" + String(appearance, HEX) + ")");
                }
            } else if (strcmp(charUUIDs[i], "2A04") == 0) {
                if (value.length() >= 8) {
                    uint16_t minInterval, maxInterval, latency, timeout;
                    memcpy(&minInterval, &value[0], sizeof(uint16_t));
                    memcpy(&maxInterval, &value[2], sizeof(uint16_t));
                    memcpy(&latency,     &value[4], sizeof(uint16_t));
                    memcpy(&timeout,     &value[6], sizeof(uint16_t));

                    accessInfoString += "Preferred Connection Parameters:\n";
                    accessInfoString += "  Min Interval: " + String(minInterval) + "\n";
                    accessInfoString += "  Max Interval: " + String(maxInterval) + "\n";
                    accessInfoString += "  Latency: " + String(latency) + "\n";
                    accessInfoString += "  Timeout: " + String(timeout) + "\n";

                    LOG(LOG_GATT, "     PPCP - Min: " + String(minInterval) + ", Max: " + String(maxInterval) + ", Latency: " + String(latency) + ", Timeout: " + String(timeout));
                }
            } else if (strcmp(charUUIDs[i], "2AA6") == 0 && !value.empty()) {
                uint8_t support = value[0];
                accessInfoString += "Central Address Resolution: " + String(support == 1 ? "Supported" : "Not Supported") + "\n";
                LOG(LOG_GATT, String("     Central Address Resolution: ") + (support == 1 ? "Supported" : "Not Supported"));
            } else {
                String val = String(value.c_str());
                accessInfoString += String(charNames[i]) + ": " + val + "\n";
                LOG(LOG_GATT, "     " + String(charNames[i]) + ": " + val);
            }
        }
    }

    return accessInfoString;
}
