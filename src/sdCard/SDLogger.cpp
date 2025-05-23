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

void SDLogger::writeDeviceInfo( const String& address, 
                                const String& localName,
                                const std::vector<std::string>& nameList,
                                const String& manuInfo,
                                const std::vector<std::string>& uuids,
                                const String& deviceInfoString, 
                                const String& batteryLevelService,
                                const String& genericAccessService) {
    if (!initialized) {
        Serial.println("#SDLogger# SDLogger not initialized.");
        return;
    }

    // Check if the file is open before writing
    if (dataFile) {

        dataFile.print("Address: ");
        dataFile.println(address);

        if (localName.length() > 0) {
            dataFile.print("Local Name: ");
            dataFile.println(localName);
        } else {
            dataFile.println("Local Name: (no name)");
        }

        dataFile.println("Device Names:");
        for (const auto& names : nameList) {
            if (!names.empty()) {
                dataFile.print("  - ");
                dataFile.println(names.c_str());
            }
        }

        //Device Info String: 
        dataFile.println(deviceInfoString);
        dataFile.println(batteryLevelService);

        dataFile.println(manuInfo);

        dataFile.println("Service and Characteristic UUIDs:");
        for (const auto& uuid : uuids) {
            dataFile.print("  - ");
            dataFile.println(uuid.c_str());
        }

        dataFile.println("-------------------------------");
        dataFile.flush();  // Make sure the data is written to the card
    } else {
        Serial.println("#SDLogger# Error writing to file. File not open.");
    }
}
