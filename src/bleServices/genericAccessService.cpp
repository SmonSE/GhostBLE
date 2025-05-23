#include "genericAccessService.h"
#include <Arduino.h>
#include <NimBLEDevice.h> 

#include "../logToSerialAndWeb/logger.h"


String GenericAccessServiceHandler::readGenericAccessInfo(NimBLEClient* pClient) {
    String accessInfoString = "";

    NimBLERemoteService* gapService = pClient->getService("1800");
    if (!gapService) {
        logToSerialAndWeb("Generic Access Service not found (0x1800)");
        return accessInfoString;
    }

    logToSerialAndWeb("Generic Access Service found (0x1800)");

    const char* charUUIDs[] = {"2A00", "2A01", "2A04", "2AA6"};
    const char* charNames[] = {
        "Device Name",
        "Appearance",
        "Preferred Connection Parameters",
        "Central Address Resolution"
    };

    for (int i = 0; i < 4; i++) {
        NimBLERemoteCharacteristic* pChar = gapService->getCharacteristic(charUUIDs[i]);
        if (!pChar) {
            Serial.printf("  Characteristic %s not found.\n", charUUIDs[i]);
            continue;
        }

        if (pChar->canRead()) {
            std::string value = pChar->readValue();

            if (strcmp(charUUIDs[i], "2A01") == 0) {
                uint16_t appearance = *(uint16_t*)value.data();
                accessInfoString += "Appearance: 0x" + String(appearance, HEX) + "\n";
                logToSerialAndWeb("  Appearance: 0x" + String(appearance, HEX));
            } else if (strcmp(charUUIDs[i], "2A04") == 0) {
                if (value.length() >= 8) {
                    uint16_t minInterval = *(uint16_t*)&value[0];
                    uint16_t maxInterval = *(uint16_t*)&value[2];
                    uint16_t latency     = *(uint16_t*)&value[4];
                    uint16_t timeout     = *(uint16_t*)&value[6];

                    accessInfoString += "Preferred Connection Parameters:\n";
                    accessInfoString += "  Min Interval: " + String(minInterval) + "\n";
                    accessInfoString += "  Max Interval: " + String(maxInterval) + "\n";
                    accessInfoString += "  Latency: " + String(latency) + "\n";
                    accessInfoString += "  Timeout: " + String(timeout) + "\n";

                    Serial.printf("  PPCP - Min: %d, Max: %d, Latency: %d, Timeout: %d\n",
                        minInterval, maxInterval, latency, timeout);
                }
            } else if (strcmp(charUUIDs[i], "2AA6") == 0) {
                uint8_t support = value[0];
                accessInfoString += "Central Address Resolution: " + String(support == 1 ? "Supported" : "Not Supported") + "\n";
                Serial.printf("  Central Address Resolution: %s\n", support == 1 ? "Supported" : "Not Supported");
            } else {
                String val = String(value.c_str());
                accessInfoString += String(charNames[i]) + ": " + val + "\n";
                Serial.printf("  %s: %s\n", charNames[i], val.c_str());
            }
        }
    }

    return accessInfoString;
}
