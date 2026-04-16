#include "heart_rate_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "globals/globals.h"
#include "helper/show_expression.h"
#include "logger/logger.h"


String HeartRateServiceHandler::readHeartRate(NimBLEClient* pClient) {
    LOG(LOG_GATT,"   Heart Rate Service");

    if (!pClient) return "";

    NimBLERemoteService* hrService = pClient->getService("180D");
    if (!hrService) {
        LOG(LOG_GATT,"     Heart Rate Service not found");
        return "";
    }

    LOG(LOG_GATT,"     Heart Rate Service found (0x180D)");

    NimBLERemoteCharacteristic* pChar = hrService->getCharacteristic("2A37");
    if (!pChar) {
        LOG(LOG_GATT,"     Heart Rate Measurement Characteristic not found");
        return "";
    }

    static bool subscribed = false;

    if (pChar->canNotify() && !subscribed) {
        subscribed = true;

        pChar->subscribe(true,
            [](NimBLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {

                if (length < 2) return;

                uint8_t flags = data[0];
                uint16_t hr = 0;

                if (flags & 0x01) {
                    if (length < 3) return;
                    hr = data[1] | (data[2] << 8);
                } else {
                    hr = data[1];
                }

                LOG(LOG_GATT, "     ❤️ Heart Rate: " + String(hr) + " bpm");
            }
        );

        LOG(LOG_GATT,"     Subscribed to Heart Rate notifications");
    }

    return "";
}
