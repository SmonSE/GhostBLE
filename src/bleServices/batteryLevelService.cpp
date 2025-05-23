#include "batteryLevelService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../globals/globals.h"
#include "../helper/showExpression.h"
#include "../logToSerialAndWeb/logger.h"


String BatteryServiceHandler::readBatteryLevel(NimBLEClient* pClient) {
  String batteryStr = "";
  logToSerialAndWeb("Battery Service");

  // Retrieve the Battery Service from the client
  NimBLERemoteService* batteryService = pClient->getService("180F");
  if (batteryService == nullptr) {
    logToSerialAndWeb("  Battery Service not found");
    return batteryStr;
  } else {
    logToSerialAndWeb("  Battery Service found (0x180F)");

    // Get the Battery Level characteristic
    NimBLERemoteCharacteristic* pChar = batteryService->getCharacteristic("2A19");
    if (pChar == nullptr) {
      logToSerialAndWeb("  Battery Level Characteristic not found");
      return batteryStr;
    }

    // Check if the characteristic is readable
    if (pChar->canRead()) {
      std::string raw = pChar->readValue();
      if (!raw.empty()) {
        uint8_t level = raw[0];
        if (level <= 100) {
          batteryStr = "  Battery Level: " + String(level) + "%\n";
          logToSerialAndWeb(batteryStr);

          if (!isThugLifeTaskRunning) {
            logToSerialAndWeb("showThugLifeExpressionTask");
            xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 3, NULL);
          }
        } else {
          batteryStr = "  Battery read failed or invalid value: " + String(level) + "\n";
          logToSerialAndWeb(batteryStr);
        }
      } else {
        batteryStr = "  Battery Level read failed or empty value";
        logToSerialAndWeb(batteryStr);
      }
    } else {
      logToSerialAndWeb("  Battery Level Characteristic not readable");
    }
  }

  return batteryStr;
}
