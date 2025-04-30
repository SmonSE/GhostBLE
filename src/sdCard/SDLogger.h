#pragma once

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <SD.h>
#include <vector>

class SDLogger {
public:
    SDLogger();
    bool begin(int csPin = -1); // Optional: provide CS pin
    void writeDeviceInfo(const String& address, const String& localName, const String& manuInfo, const String& targetMessage, const String& mainUuidStr, const String& deviceInfoString);

private:
    File dataFile;
    bool initialized;
};
