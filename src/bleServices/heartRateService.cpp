#include "heartRateService.h"

// ✅ Include needed NimBLE headers
#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../globals/globals.h"
#include "../helper/showExpression.h"
#include "../logToSerialAndWeb/logger.h"


String HeartRateServiceHandler::readHeartRate(NimBLEClient* pClient) {
  String hrStr = "";
  logToSerialAndWeb("   Heart Rate Service");

  // Retrieve the Heart Rate Service from the client
  NimBLERemoteService* hrService = pClient->getService("180D");
  if (hrService == nullptr) {
    logToSerialAndWeb("     Heart Rate Service not found");
    return hrStr;
  } else {
    logToSerialAndWeb("     Heart Rate Service found (0x180D)");

    // Get the Heart Rate Measurement characteristic
    NimBLERemoteCharacteristic* pChar = hrService->getCharacteristic("2A37");
    if (pChar == nullptr) {
      logToSerialAndWeb("     Heart Rate Measurement Characteristic not found");
      return hrStr;
    }

    if (pChar->canNotify()) {
      pChar->subscribe(true, [](NimBLERemoteCharacteristic* chr, uint8_t* data, size_t length, bool isNotify) {
        if (length > 1) {
          uint8_t flags = data[0];
          uint16_t hrValue = data[1];
          if (flags & 0x01 && length >= 3) {
            hrValue = (uint16_t)data[1] | (data[2] << 8);
          }
          String hrStr = "     ❤️ Heart Rate Notification: " + String(hrValue) + " bpm";
          logToSerialAndWeb(hrStr);
          // You can also trigger your display task here if needed
          if (!isThugLifeTaskRunning) {
            logToSerialAndWeb("showThugLifeExpressionTask");
            xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 3, NULL);
          }
        }
      });
      logToSerialAndWeb("     Subscribed to Heart Rate notifications");
    } else {
      logToSerialAndWeb("     Heart Rate Characteristic does not support notifications");
    }
  }
  
  return hrStr;
}
