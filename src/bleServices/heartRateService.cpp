#include "heartRateService.h"
#include <ArduinoBLE.h>

String HeartRateServiceHandler::readHeartRate(BLEDevice peripheral) {
  String heartRateStr = "";

  BLEService hrService = peripheral.service("180D");  // Heart Rate Service UUID
  if (hrService) {
    Serial.println("Heart Rate Service found (0x180D)");

    BLECharacteristic hrChar = hrService.characteristic("2A37");  // Heart Rate Measurement UUID

    if (hrChar) {
      // Subscribe to the characteristic (no callback function)
      if (hrChar.subscribe()) {
        Serial.println("Subscribed to notifications for heart rate");
      } else {
        Serial.println("Failed to subscribe to notifications for heart rate");
      }

      // Wait to receive notifications (polling)
      delay(500);  // Adjust this delay as necessary

      // Check if a value has been received
      if (hrChar.canRead()) {
        uint8_t buffer[20];  // Create a buffer to hold the data
        int len = hrChar.readValue(buffer, sizeof(buffer));
        if (len > 1) {
          int bpm = buffer[1];  // Heart rate is usually in the second byte
          heartRateStr = String(bpm);
          Serial.print("Heart Rate: ");
          Serial.println(bpm);
        }
      }
    }
  }

  return heartRateStr;
}
