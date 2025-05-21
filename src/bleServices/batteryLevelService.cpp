#include "batteryLevelService.h"

// ✅ Include needed NimBLE headers
#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../globals/globals.h"
#include "../helper/showExpression.h"

String BatteryServiceHandler::readBatteryLevel(NimBLEClient* pClient) {
  String batteryStr = "";

  // Retrieve the Battery Service from the client
  NimBLERemoteService* batteryService = pClient->getService("180F");
  if (batteryService == nullptr) {
    Serial.println("  Battery Service not found");
    return batteryStr;
  } else {
    Serial.println("  Battery Service found (0x180F)");

    // Get the Battery Level characteristic
    NimBLERemoteCharacteristic* pChar = batteryService->getCharacteristic("2A19");
    if (pChar == nullptr) {
      Serial.println("  Battery Level Characteristic not found");
      return batteryStr;
    }

    // Check if the characteristic is readable
    if (pChar->canRead()) {
      std::string raw = pChar->readValue();
      if (!raw.empty()) {
        uint8_t level = raw[0];
        if (level <= 100) {
          batteryStr = "  Battery Level: " + String(level) + "%\n";
          Serial.println(batteryStr);

          if (!isThugLifeTaskRunning) {
            Serial.println("showThugLifeExpressionTask");
            xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 3, NULL);
          }
        } else {
          batteryStr = "  Battery read failed or invalid value: " + String(level) + "\n";
          Serial.println(batteryStr);
        }
      } else {
        batteryStr = "  Battery Level read failed or empty value";
        Serial.println(batteryStr);
      }
    } else {
      Serial.println("  Battery Level Characteristic not readable");
    }
  }

  return batteryStr;
}
