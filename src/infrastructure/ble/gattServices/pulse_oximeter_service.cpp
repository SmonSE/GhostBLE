#include "pulse_oximeter_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"


String PulseOximeterServiceHandler::readSpO2(NimBLEClient* pClient) {
    LOG(LOG_GATT, "   Pulse Oximeter Service");

    if (!pClient) return "";

    NimBLERemoteService* svc = pClient->getService("1822");
    if (!svc) {
        LOG(LOG_GATT, "     SpO2 Service not found");
        return "";
    }

    LOG(LOG_GATT, "     SpO2 Service found (0x1822)");

    NimBLERemoteCharacteristic* chr = svc->getCharacteristic("2A5F");
    if (!chr) {
        LOG(LOG_GATT, "     SpO2 Measurement Characteristic not found");
        return "";
    }

    static bool subscribed = false;

    if (chr->canNotify() && !subscribed) {
        subscribed = true;

        chr->subscribe(true,
            [](NimBLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {
                PulseOximeterServiceHandler::handleNotification(data, length);
            }
        );

        LOG(LOG_GATT, "     Subscribed to SpO2 notifications");
    }

    return "";
}


void PulseOximeterServiceHandler::handleNotification(uint8_t* data, size_t length) {
    if (length < 4) return;

    uint8_t flags = data[0];

    // According to spec:
    // SpO2 and pulse are SFLOAT (16-bit IEEE-11073)
    uint16_t spo2Raw = data[1] | (data[2] << 8);
    uint16_t pulseRaw = data[3] | (data[4] << 8);

    // Simple conversion (most devices use integer values)
    float spo2 = spo2Raw * 0.1f;
    float pulse = pulseRaw * 0.1f;

    LOG(LOG_GATT, "🫁 SpO2: " + String(spo2) + " %");
    LOG(LOG_GATT, "❤️ Pulse: " + String(pulse) + " bpm");

    // Optional fields (flags-based, often not present)
    int index = 5;

    if (flags & 0x01) {
        // fast measurement present
        index += 4;
    }

    if (flags & 0x02) {
        // slow measurement present
        index += 4;
    }

    if (flags & 0x04) {
        // measurement status
        index += 2;
    }

    if (flags & 0x08) {
        // device and sensor status
        index += 3;
    }

    if (flags & 0x10) {
        // pulse amplitude index
        index += 2;
    }
}