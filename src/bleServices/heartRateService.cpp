#include "heartRateService.h"

String HeartRateServiceHandler::readHeartRate(BLEDevice peripheral) {
  String heartRateStr = "";

  BLEService hrService = peripheral.service("180D");
  if (hrService) {
    Serial.println("Heart Rate Service found (0x180D)");
    BLECharacteristic hrChar = hrService.characteristic("2A37");
    if (hrChar) {
      delay(100);  // <-- Kurze Pause, damit das Gerät den Wert bereitstellen kann
      if (hrChar.canRead()) {
        uint8_t buffer[8];
        int len = hrChar.readValue(buffer, sizeof(buffer));
        if (len > 1) {
          int bpm = buffer[1];
          heartRateStr = String(bpm);
          Serial.print("  Heart Rate: ");
          Serial.println(bpm);
        }
      }
    }
  }

  return heartRateStr;
}
