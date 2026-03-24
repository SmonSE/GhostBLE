#include "alertNotificationService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

String AlertNotificationServiceHandler::readAlertNotification(NimBLEClient* pClient) {
    String result = "";

    if (!pClient) return result;

    NimBLERemoteService* service = pClient->getService("1811");
    if (!service) return result;

    LOG(LOG_GATT, "     Alert Notification Service detected (0x1811)");

    // Supported New Alert Category (0x2A47)
    NimBLERemoteCharacteristic* pCat = service->getCharacteristic("2A47");
    if (pCat && pCat->canRead()) {
        std::string raw = pCat->readValue();
        if (!raw.empty()) {
            uint8_t categories = raw[0];
            result = "Alert Categories: ";

            const char* catNames[] = {
                "Simple Alert", "Email", "News", "Call",
                "Missed Call", "SMS/MMS", "Voice Mail", "Schedule"
            };
            bool first = true;
            for (int i = 0; i < 8; i++) {
                if (categories & (1 << i)) {
                    if (!first) result += ", ";
                    result += catNames[i];
                    first = false;
                }
            }
            result += "\n";
            LOG(LOG_GATT, "     " + result);
        }
    }

    if (result.isEmpty()) {
        result = "Alert Notification: Present\n";
    }

    return result;
}
