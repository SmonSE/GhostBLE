#pragma once

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <SD.h>
#include <vector>

class SDLogger {
public:
    SDLogger();
    bool begin(int csPin = -1); // Optional: provide CS pin
    void writeDeviceInfo(const String& address, const String& localName, BLEDevice& peripheral, const std::vector<String>& serviceUuids);

private:
    File dataFile;
    bool initialized;
};
