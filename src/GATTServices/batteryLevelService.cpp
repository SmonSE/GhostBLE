#include "batteryLevelService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../globals/globals.h"
#include "../helper/showExpression.h"
#include "../logToSerialAndWeb/logger.h"

String BatteryServiceHandler::readBatteryLevel(NimBLEClient* pClient) {
  String batteryStr = "";
  Serial.println("   Battery Service");

  // Check for null client
  if (pClient == nullptr) {
    Serial.println("     ⚠️ pClient is null.");
    return batteryStr;
  }

  // Retrieve the Battery Service (UUID: 0x180F)
  NimBLERemoteService* batteryService = pClient->getService("180F");
  if (batteryService == nullptr) {
    Serial.println("     Battery Service not found");
    return batteryStr;
  } else {
    Serial.println("     Battery Service found (0x180F)");

    // Retrieve the Battery Level Characteristic (UUID: 0x2A19)
    NimBLERemoteCharacteristic* pChar = batteryService->getCharacteristic("2A19");
    if (pChar == nullptr) {
      Serial.println("     Battery Level Characteristic not found");
      return batteryStr;
    }

    // Check if characteristic is readable
    if (pChar->canRead()) {
      std::string raw = pChar->readValue();

      if (!raw.empty()) {
        if (raw.size() >= 1) {
          uint8_t level = static_cast<uint8_t>(raw[0]);

          if (level <= 100) {
            batteryStr = "     Battery Level: " + String(level);
            Serial.println(batteryStr);

            leakedCounter++;

            if (!isThugLifeTaskRunning) {
              //logToSerialAndWeb("showThugLifeExpressionTask");
              xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 3, NULL);
            }
          } else {
            batteryStr = "     Battery read failed or invalid value: " + String(level);
            Serial.println(batteryStr);
          }
        } else {
          batteryStr = "     ⚠️ Battery data too short\n";
          Serial.println(batteryStr);
        }
      } else {
        batteryStr = "     ⚠️ Failed to read battery level (empty response)";
        Serial.println(batteryStr);
      }
    } else {
      Serial.println("     Battery Level Characteristic not readable");

      // Optional: suggest using notification if available
      if (pChar->canNotify()) {
        Serial.println("     Battery Level Characteristic supports notify, consider subscribing.");
      }
    }
  }

  return batteryStr;
}
