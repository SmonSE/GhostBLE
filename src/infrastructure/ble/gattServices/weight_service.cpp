#include "weight_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"


String WeightServiceHandler::readWeight(NimBLEClient* pClient) {
    LOG(LOG_GATT, "   Weight Scale Service");

    if (!pClient) return "";

    NimBLERemoteService* svc = pClient->getService("181D");
    if (!svc) {
        LOG(LOG_GATT, "     Weight Service not found");
        return "";
    }

    LOG(LOG_GATT, "     Weight Service found (0x181D)");

    NimBLERemoteCharacteristic* chr = svc->getCharacteristic("2A9D");
    if (!chr) {
        LOG(LOG_GATT, "     Weight Measurement Characteristic not found");
        return "";
    }

    static bool subscribed = false;

    if (chr->canNotify() && !subscribed) {
        subscribed = true;

        chr->subscribe(true,
            [](NimBLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {
                WeightServiceHandler::handleNotification(data, length);
            }
        );

        LOG(LOG_GATT, "     Subscribed to Weight notifications");
    }

    return "";
}


void WeightServiceHandler::handleNotification(uint8_t* data, size_t length) {
    if (length < 3) return;

    uint8_t flags = data[0];
    uint16_t rawWeight = data[1] | (data[2] << 8);

    // Unit handling
    float weight = 0.0f;
    if (flags & 0x01) {
        // pounds
        weight = rawWeight * 0.01f;
        LOG(LOG_GATT, "⚖️ Weight: " + String(weight) + " lb");
    } else {
        // kilograms
        weight = rawWeight * 0.005f;
        LOG(LOG_GATT, "⚖️ Weight: " + String(weight) + " kg");
    }

    int index = 3;

    // Timestamp (7 bytes)
    if (flags & 0x02) {
        if (length >= index + 7) {
            // optional: parse timestamp if needed
            index += 7;
        } else {
            return;
        }
    }

    // User ID (1 byte)
    if (flags & 0x04) {
        if (length >= index + 1) {
            uint8_t userId = data[index++];
            LOG(LOG_GATT, "👤 User ID: " + String(userId));
        } else {
            return;
        }
    }

    // BMI (2 bytes) + Height (2 bytes)
    if (flags & 0x08) {
        if (length >= index + 4) {
            uint16_t bmiRaw = data[index] | (data[index + 1] << 8);
            uint16_t heightRaw = data[index + 2] | (data[index + 3] << 8);

            float bmi = bmiRaw * 0.1f;
            float height = heightRaw * 0.01f;

            LOG(LOG_GATT, "📊 BMI: " + String(bmi));
            LOG(LOG_GATT, "📏 Height: " + String(height) + " m");
        }
    }
}