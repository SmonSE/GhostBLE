#include "env_sensing_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "logger/logger.h"

String EnvironmentalSensingServiceHandler::readEnvironmentalSensing(NimBLEClient* pClient) {
    String result = "";

    if (!pClient) return result;

    NimBLERemoteService* service = pClient->getService("181A");
    if (!service) return result;

    LOG(LOG_GATT, "     Environmental Sensing Service detected (0x181A)");

    // Temperature (0x2A6E) — sint16, units 0.01 °C
    NimBLERemoteCharacteristic* pTemp = service->getCharacteristic("2A6E");
    if (pTemp && pTemp->canRead()) {
        std::string raw = pTemp->readValue();
        if (raw.size() >= 2) {
            int16_t tempRaw = (uint8_t)raw[0] | ((uint8_t)raw[1] << 8);
            float tempC = tempRaw / 100.0f;
            result += "🌡 Temperature: " + String(tempC, 1) + " °C\n";
            LOG(LOG_GATT, "     Temperature: " + String(tempC, 1) + " °C");
        }
    }

    // Humidity (0x2A6F) — uint16, units 0.01 %
    NimBLERemoteCharacteristic* pHum = service->getCharacteristic("2A6F");
    if (pHum && pHum->canRead()) {
        std::string raw = pHum->readValue();
        if (raw.size() >= 2) {
            uint16_t humRaw = (uint8_t)raw[0] | ((uint8_t)raw[1] << 8);
            float humidity = humRaw / 100.0f;
            result += "💧 Humidity: " + String(humidity, 1) + " %\n";
            LOG(LOG_GATT, "     Humidity: " + String(humidity, 1) + " %");
        }
    }

    // Pressure (0x2A6D) — uint32, units 0.1 Pa
    NimBLERemoteCharacteristic* pPres = service->getCharacteristic("2A6D");
    if (pPres && pPres->canRead()) {
        std::string raw = pPres->readValue();
        if (raw.size() >= 4) {
            uint32_t presRaw = (uint8_t)raw[0] | ((uint8_t)raw[1] << 8) |
                               ((uint8_t)raw[2] << 16) | ((uint8_t)raw[3] << 24);
            float presHpa = presRaw / 1000.0f;
            result += "🌀 Pressure: " + String(presHpa, 1) + " hPa\n";
            LOG(LOG_GATT, "     Pressure: " + String(presHpa, 1) + " hPa");
        }
    }

    if (result.isEmpty()) {
        result = "Environmental Sensing: Present\n";
    }

    return result;
}
