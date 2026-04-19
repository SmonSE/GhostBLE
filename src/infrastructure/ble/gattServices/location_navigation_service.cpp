#include "location_navigation_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"


String LocationNavigationServiceHandler::readLocation(NimBLEClient* pClient) {
    LOG(LOG_GATT, "   Location & Navigation Service");

    if (!pClient) return "";

    NimBLERemoteService* svc = pClient->getService("1819");
    if (!svc) {
        LOG(LOG_GATT, "     Location Service not found");
        return "";
    }

    LOG(LOG_GATT, "     Location Service found (0x1819)");

    NimBLERemoteCharacteristic* chr = svc->getCharacteristic("2A67");
    if (!chr) {
        LOG(LOG_GATT, "     Location & Speed Characteristic not found");
        return "";
    }

    static bool subscribed = false;

    if (chr->canNotify() && !subscribed) {
        subscribed = true;

        chr->subscribe(true,
            [](NimBLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {
                LocationNavigationServiceHandler::handleNotification(data, length);
            }
        );

        LOG(LOG_GATT, "     Subscribed to Location notifications");
    }

    return "";
}


void LocationNavigationServiceHandler::handleNotification(uint8_t* data, size_t length) {
    if (length < 1) return;

    uint16_t flags = data[0] | (data[1] << 8);
    int index = 2;

    LOG(LOG_GATT, "🧭 Location packet received");

    // Latitude (4 bytes, sint32, degrees * 1e7)
    if (flags & 0x0001) {
        if (length >= index + 4) {
            int32_t rawLat = data[index] | (data[index+1] << 8) |
                             (data[index+2] << 16) | (data[index+3] << 24);
            float lat = rawLat / 1e7f;
            LOG(LOG_GATT, "📍 Latitude: " + String(lat, 6));
            index += 4;
        } else return;
    }

    // Longitude (4 bytes)
    if (flags & 0x0002) {
        if (length >= index + 4) {
            int32_t rawLon = data[index] | (data[index+1] << 8) |
                             (data[index+2] << 16) | (data[index+3] << 24);
            float lon = rawLon / 1e7f;
            LOG(LOG_GATT, "📍 Longitude: " + String(lon, 6));
            index += 4;
        } else return;
    }

    // Elevation (3 bytes, meters)
    if (flags & 0x0004) {
        if (length >= index + 3) {
            int32_t rawElev = data[index] | (data[index+1] << 8) |
                              (data[index+2] << 16);
            float elevation = rawElev * 0.01f;
            LOG(LOG_GATT, "⛰ Elevation: " + String(elevation) + " m");
            index += 3;
        } else return;
    }

    // Speed (2 bytes, m/s)
    if (flags & 0x0008) {
        if (length >= index + 2) {
            uint16_t rawSpeed = data[index] | (data[index+1] << 8);
            float speed = rawSpeed * 0.01f;
            LOG(LOG_GATT, "🚀 Speed: " + String(speed) + " m/s");
            index += 2;
        } else return;
    }

    // Heading (2 bytes, degrees)
    if (flags & 0x0010) {
        if (length >= index + 2) {
            uint16_t rawHeading = data[index] | (data[index+1] << 8);
            float heading = rawHeading * 0.01f;
            LOG(LOG_GATT, "🧭 Heading: " + String(heading) + "°");
            index += 2;
        } else return;
    }
}