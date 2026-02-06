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
                         const std::vector<std::string>& nameList,
                         const String& manuInfo,
                         const String& deviceInfoString, 
                         const String& batteryLevelService,
                         const String& genericAccessService);

    void writeIBeaconInfo(
        const String& uuid,
        const String& major,
        const String& minor,
        const String& distance,
        const String& manufacturerName,
        uint16_t manufacturerId,
        int rssi);

private:
    File dataFile;
    bool initialized;
};
