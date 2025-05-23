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
  logToSerialAndWeb("Heart Rate Service");

  // Retrieve the Heart Rate Service from the client
  NimBLERemoteService* hrService = pClient->getService("180D");
  if (hrService == nullptr) {
    logToSerialAndWeb("  Heart Rate Service not found");
    return hrStr;
  } else {
    logToSerialAndWeb("  Heart Rate Service found (0x180D)");

    // Get the Heart Rate Measurement characteristic
    NimBLERemoteCharacteristic* pChar = hrService->getCharacteristic("2A37");
    if (pChar == nullptr) {
      logToSerialAndWeb("  Heart Rate Measurement Characteristic not found");
      return hrStr;
    }

    // Check if the characteristic is readable (Note: usually it's not)
    if (pChar->canRead()) {
      std::string raw = pChar->readValue();
      if (!raw.empty()) {
        uint8_t flags = raw[0];
        uint8_t hrValue = raw[1];

        // Optional: handle 16-bit HR values if flags indicate
        if (flags & 0x01 && raw.size() >= 3) {
          hrValue = (uint16_t)raw[1] | (raw[2] << 8);
        }

        hrStr = "  Heart Rate: " + String(hrValue) + " bpm\n";
        logToSerialAndWeb(hrStr);
                  
        if (!isThugLifeTaskRunning) {
          logToSerialAndWeb("showThugLifeExpressionTask");
          xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 3, NULL);
        }
      } else {
        hrStr = "  Heart Rate read failed or empty value";
        logToSerialAndWeb(hrStr);
      }
    } else {
      logToSerialAndWeb("  Heart Rate Characteristic not readable (usually not, use notifications)");
    }
  }

  return hrStr;
}
