#include "heartRateService.h"  // Include the header file

String HeartRateServiceHandler::readHeartRate(NimBLEClient* pClient) {
  String heartRateStr = "";

  // Get the heart rate service
  NimBLERemoteService* hrService = pClient->getService("180D");
  if (hrService != nullptr) {
    Serial.println("Heart Rate Service found (0x180D)");

    // Get the heart rate characteristic
    NimBLERemoteCharacteristic* hrChar = hrService->getCharacteristic("2A37");
    if (hrChar != nullptr) {
      delay(100);  // Short delay to allow the device to respond

      // Check if the characteristic can be read
      if (hrChar->canRead()) {
        NimBLEAttValue result = hrChar->readValue();
        
        // Check if the read operation was successful
        if (result.length() > 1) {
          // Assuming heart rate is in the second byte
          int bpm = result[1];  
          heartRateStr = String(bpm);
          Serial.print("  Heart Rate: ");
          Serial.println(bpm);
        }
      }
    }
  }

  return heartRateStr;
}
