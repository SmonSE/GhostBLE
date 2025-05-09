#include "deviceInfoService.h"


#include "deviceInfoService.h"

String DeviceInfoServiceHandler::readDeviceInfo(NimBLEClient* pClient) {
    String deviceInfoString = "";

    NimBLERemoteService* deviceInfoService = pClient->getService("180A");
    if (deviceInfoService) {
        Serial.println("Device Information Service found (0x180A)");

        const char* deviceChars[] = {"2A29", "2A24", "2A25", "2A27", "2A26", "2A28"};
        const char* charNames[]   = {"Manufacturer Name", "Model Number", "Serial Number", "Hardware Revision", "Firmware Revision", "Software Revision"};

        for (int i = 0; i < 6; i++) {
          NimBLERemoteCharacteristic* pChar = deviceInfoService->getCharacteristic(deviceChars[i]);
          if (pChar == nullptr) {
              Serial.printf("❌ Characteristic %s not found.\n", deviceChars[i]);
              continue;
          }
      
          if (pChar->canRead()) {
              std::string value = pChar->readValue();
              String val = String(value.c_str());
              deviceInfoString += String(charNames[i]) + ": " + val + "\n";
      
              Serial.printf("  %s: %s\n", charNames[i], val.c_str());
          }
        }
      }

    return deviceInfoString;
}
