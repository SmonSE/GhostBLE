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
