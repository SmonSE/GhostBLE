#pragma once

#include <Arduino.h>
#include <SD.h>
#include <vector>

class SDLogger {
public:
    SDLogger();
    bool begin(int csPin = -1); // Optional: provide CS pin
    void writeDeviceInfo(const String& address, 
                         const String& localName, 
                         const String& riskLevelSdCard,
                         const std::vector<std::string>& nameList,
                         const String& manuInfo,
                         const std::vector<std::string>& uuids,
                         const String& deviceInfoString, 
                         const String& batteryLevelService,
                         const String& genericAccessService);

private:
    File dataFile;
    bool initialized;
};
