#include "temperature_service.h"

#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "config/gatt_config.h"
#include "app/context/globals.h"
#include "infrastructure/logging/logger.h"

static bool tempSubscribed = false;

/*
  BLE Health Thermometer Service
  Service UUID:        0x1809
  Measurement Char:    0x2A1C
  Format: IEEE-11073 FLOAT (Celsius)
*/

static float parseIEEE11073Float(const uint8_t* data) {
    // 32-bit FLOAT: [ exponent (8bit signed) | mantissa (24bit signed) ]
    int32_t mantissa =
        (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16));
    if (mantissa & 0x800000) mantissa |= 0xFF000000; // sign extend

    int8_t exponent = (int8_t)data[3];

    return mantissa * pow(10.0f, exponent);
}

String TemperatureServiceHandler::readTemperature(NimBLEClient* pClient) {
    LOG(LOG_GATT,"   🌡️ Temperature Service");

    if (!pClient) return "";

    NimBLERemoteService* tempService = pClient->getService(UUID_HEALTH_THERMOMETER);
    if (!tempService) {
        LOG(LOG_GATT,"     Temperature Service not found (0x1809)");
        return "";
    }

    LOG(LOG_GATT,"     Temperature Service found (0x1809)");

    NimBLERemoteCharacteristic* pChar = tempService->getCharacteristic("2A1C");
    if (!pChar) {
        LOG(LOG_GATT,"     Temperature Measurement Characteristic not found (0x2A1C)");
        return "";
    }

    if (pChar->canNotify() && !tempSubscribed) {
        tempSubscribed = true;

        pChar->subscribe(true,
            [](NimBLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {

                if (length < 5) return;  // flags + float32

                uint8_t flags = data[0];
                const uint8_t* tempRaw = &data[1];

                float temperature = parseIEEE11073Float(tempRaw);

                String unit = (flags & 0x01) ? "F" : "C";

                LOG(LOG_GATT, "     🌡️ Temperature: " + String(temperature, 2) + " °" + unit);

                LOG(LOG_GATT,
                    "🌡️ Temperature: " + String(temperature, 2) + " °" + unit
                );
            }
        );

        LOG(LOG_GATT,"     Subscribed to temperature notifications");
    }
    else if (!pChar->canNotify()) {
        LOG(LOG_GATT,"     Temperature characteristic does NOT support notify");
    }

    return "";
}
