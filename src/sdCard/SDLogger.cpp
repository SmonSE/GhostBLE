#include "SDLogger.h"
#include "../helper/ServiceHelper.h"  // Assuming you still need service name mapping


SDLogger::SDLogger() : initialized(false) {}

bool SDLogger::begin(int csPin) {
    // Initialize SD card with the correct chip select pin
    if (csPin != -1) {
        if (!SD.begin(csPin)) {
            Serial.println("#SDLogger# SD card initialization failed!");
            return false;
        }
    } else {
        if (!SD.begin()) {
            Serial.println("#SDLogger# SD card initialization failed!");
            return false;
        }
    }

    // Try opening the file with a check for success
    dataFile = SD.open("/device_info.txt", FILE_APPEND);  // Try without leading "/"
    if (!dataFile) {
        Serial.println("#SDLogger# Error opening /device_info.txt for writing.");
        return false;
    }

    // Log success and set initialization flag
    Serial.println("#SDLogger# /device_info.txt opened successfully.");
    initialized = true;
    return true;
}

void SDLogger::writeDeviceInfo(const String& address, const String& localName, const String& manuInfo, const String& targetMessage, const String& serviceInfo, const String& deviceInfoString) {
    if (!initialized) {
        Serial.println("#SDLogger# SDLogger not initialized.");
        return;
    }

    // Check if the file is open before writing
    if (dataFile) {
        // Discovered Service UUID: 
        dataFile.println(serviceInfo);

        dataFile.print("Address: ");
        dataFile.println(address);

        if (localName.length() > 0) {
            dataFile.print("Local Name: ");
            dataFile.println(localName);
        } else {
            dataFile.println("Local Name: (no name)");
        }

        //Device Info String: 
        dataFile.println(deviceInfoString);

        //Manufacturer ID: 
        dataFile.println(manuInfo);

        // Target Message:
        dataFile.println(targetMessage);

        dataFile.println("-------------------------------");
        dataFile.flush();  // Make sure the data is written to the card
    } else {
        Serial.println("#SDLogger# Error writing to file. File not open.");
    }
}
