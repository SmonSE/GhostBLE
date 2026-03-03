#pragma once

#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "../analyzer/ExposureAnalyzer.h"

class SDLogger {
public:
    SDLogger();
    bool begin(int csPin = -1); // Optional: provide CS pin
    void writeDeviceInfo(const String& address, 
                         const String& localName, 
                         const std::vector<std::string>& nameList,
                         const String& deviceInfoString);

    void writeIBeaconInfo(
        const String& uuid,
        const String& major,
        const String& minor,
        const String& distance,
        const String& manufacturerName,
        int rssi);

    void writeUncovered(const ExposureResult& exposure);

    void writeCategory(const String& category);

private:
    File dataFile;
    bool initialized;
};
