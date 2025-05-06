#include "deviceInfoService.h"

String DeviceInfoServiceHandler::readDeviceInfo(BLEDevice peripheral) {
  String deviceInfoString = "";

  BLEService deviceInfoService = peripheral.service("180A");
  if (deviceInfoService) {
    Serial.println("Device Information Service found (0x180A)");
    const char* deviceChars[] = {"2A29", "2A24", "2A25", "2A27", "2A26", "2A28"};
    const char* charNames[]   = {"Manufacturer Name", "Model Number", "Serial Number", "Hardware Revision", "Firmware Revision", "Software Revision"};

    for (int i = 0; i < 6; i++) {
      BLECharacteristic c = deviceInfoService.characteristic(deviceChars[i]);
      if (c && c.canRead()) {
        delay(100);  // <-- Kurze Pause, damit das Gerät den Wert bereitstellen kann
        uint8_t buffer[32];
        int len = c.readValue(buffer, sizeof(buffer));
        if (len > 0) {
          String val = "";
          for (int k = 0; k < len; k++) {
            val += (char)buffer[k];
          }
          deviceInfoString += String(charNames[i]) + ": " + val + "\n";
          Serial.print("  Value: ");
          Serial.println(val);
        } 
      } 
    }
  }

  return deviceInfoString;
}

String DeviceInfoServiceHandler::readGenericAccessInfo(BLEDevice peripheral) {
  String genericAccessInfoString = "";

  BLEService genericAccessService = peripheral.service("1800");
  if (genericAccessService) {
    Serial.println("Generic Access Service found (0x1800)");
    const char* accessChars[] = {"2A00", "2A01"};
    const char* accessNames[] = {"Device Name", "Appearance"};

    for (int i = 0; i < 2; i++) {
      BLECharacteristic c = genericAccessService.characteristic(accessChars[i]);
      if (c && c.canRead()) {
        delay(100);  // Kurze Pause
        uint8_t buffer[32];
        int len = c.readValue(buffer, sizeof(buffer));
        if (len > 0) {
          String val = "";

          if (strcmp(accessChars[i], "2A01") == 0 && len >= 2) {
            // Appearance ist ein 16-bit Integer
            uint16_t appearance = buffer[0] | (buffer[1] << 8);
            val = "0x" + String(appearance, HEX);
          } else {
            for (int k = 0; k < len; k++) {
              val += (char)buffer[k];
            }
          }

          genericAccessInfoString += String(accessNames[i]) + ": " + val + "\n";
          Serial.print("  Value: ");
          Serial.println(val);
        }
      }
    }
  }

  return genericAccessInfoString;
}
