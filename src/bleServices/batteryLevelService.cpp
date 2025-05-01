#include "batteryLevelService.h"

String BatteryServiceHandler::readBatteryLevel(BLEDevice peripheral) {
  String batteryStr = "";

  BLEService batteryService = peripheral.service("180F");
  if (batteryService) {
    Serial.println("Battery Service found (0x180F)");

    BLECharacteristic c = batteryService.characteristic("2A19");
    if (c && c.canRead()) {
      delay(100);  // <-- Kurze Pause, damit das Gerät den Wert bereitstellen kann
      uint8_t level;
      c.readValue(&level, 1);
      batteryStr += "  Battery Level: " + String(level) + "%\n";
      Serial.println(batteryStr);
    } else {
      Serial.println("  Battery Level Characteristic not readable");
    }
  }

  return batteryStr;
}
