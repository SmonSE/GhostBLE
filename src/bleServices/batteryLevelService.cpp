#include "batteryLevelService.h"

String BatteryServiceHandler::readBatteryLevel(BLEDevice peripheral) {
  String batteryStr = "";

  BLEService batteryService = peripheral.service("180F");
  if (batteryService) {
    Serial.println("Battery Service found (0x180F)");

    BLECharacteristic c = batteryService.characteristic("2A19");
    if (c && c.canRead()) {
      delay(200);  // etwas mehr Zeit
      uint8_t level = 0;
      int bytesRead = c.readValue(&level, 1);
    
      Serial.print("  Bytes read: ");
      Serial.println(bytesRead);
    
      Serial.print("  Raw level: ");
      Serial.println(level);
    
      if (bytesRead == 1 && level <= 100) {
        batteryStr = "  Battery Level: " + String(level) + "%\n";
        Serial.println(batteryStr);
      } else {
        batteryStr = "  Battery read failed or invalid value: " + String(level) + "\n";
        Serial.println(batteryStr);
      }    
    } else {
      Serial.println("  Battery Level Characteristic not readable");
    }
  }

  return batteryStr;
}
