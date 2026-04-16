#include "deviceInfoService.h"
#include "logger/logger.h"


String DeviceInfoServiceHandler::readDeviceInfo(NimBLEClient* pClient) {
    String deviceInfoString = "";

    NimBLERemoteService* deviceInfoService = pClient->getService("180A");
    if (deviceInfoService) {
        //LOG(LOG_GATT, "   Device Information Service found (0x180A)");

        const char* deviceChars[] = {"2A29", "2A24", "2A25", "2A27", "2A26", "2A28"};
        const char* charNames[]   = {"   Manufacturer Name", "   Model Number", "   Serial Number", "   Hardware Revision", "   Firmware Revision", "   Software Revision"};

        for (int i = 0; i < 6; i++) {
          NimBLERemoteCharacteristic* pChar = deviceInfoService->getCharacteristic(deviceChars[i]);
            if (pChar == nullptr) {
                LOG(LOG_GATT, "     Characteristic " + String(deviceChars[i]) + " not found.");
                continue;
            }
      
            if (pChar->canRead()) {
                std::string val = pChar->readValue();
                String valueStr = String(val.c_str());

                String line = String(charNames[i]) + ": " + valueStr;

                //deviceInfoString += line + "\n";
                LOG(LOG_GATT, "     " + line);
            }
        }
      } 

    return deviceInfoString;
}
