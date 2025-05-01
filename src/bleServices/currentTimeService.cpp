#include "currentTimeService.h"

String CurrentTimeServiceHandler::readCurrentTime(BLEDevice peripheral) {
  String timeStr = "";

  BLEService timeService = peripheral.service("1805");  // Current Time Service
  if (timeService) {
    Serial.println("Current Time Service found (0x1805)");
    BLECharacteristic timeChar = timeService.characteristic("2A2B");  // Current Time characteristic

    if (timeChar && timeChar.canRead()) {
      delay(100);  // Give the peripheral some time to prepare
      uint8_t buffer[10];
      int len = timeChar.readValue(buffer, sizeof(buffer));

      if (len >= 7) {  // At least 7 bytes needed for date/time
        int year  = buffer[0] | (buffer[1] << 8);
        int month = buffer[2];
        int day   = buffer[3];
        int hour  = buffer[4];
        int min   = buffer[5];
        int sec   = buffer[6];

        char formatted[64];
        snprintf(formatted, sizeof(formatted), "  Current Time: %04d-%02d-%02d %02d:%02d:%02d",
                 year, month, day, hour, min, sec);

        timeStr = String(formatted);
        Serial.println(timeStr);
      } else {
        Serial.print("  Failed to read enough bytes for current time length: ");
        Serial.println(len);
        /*
        Serial.print("  Raw bytes: ");
        for (int i = 0; i < len; i++) {
          Serial.print("0x");
          if (buffer[i] < 16) Serial.print("0");
          Serial.print(buffer[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        */
      }
    } else {
      Serial.println("  Current Time characteristic not readable.");
    }
  }

  return timeStr;
}
