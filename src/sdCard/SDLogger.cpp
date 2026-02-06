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

        //Device Info String: 
        dataFile.println(deviceInfoString);
        dataFile.println(batteryLevelService);

        dataFile.println(manuInfo);

        dataFile.println("Characteristic NAME:");
        for (const auto& names : nameList) {
            if (!names.empty()) {
                dataFile.print("  - ");
                dataFile.println(names.c_str());
            }
        }

        dataFile.println("-------------------------------");
        dataFile.flush();  // Make sure the data is written to the card
    } else {
        Serial.println("#SDLogger# Error writing to file. File not open.");
    }
}

void SDLogger::writeIBeaconInfo(
    const String& uuid,
    const String& major,
    const String& minor,
    const String& distance,
    const String& manufacturerName,
    uint16_t manufacturerId,
    int rssi)
{
    if (!initialized || !dataFile) return;

    dataFile.println("---- iBeacon ----");

    dataFile.print("UUID: ");
    dataFile.println(uuid);

    dataFile.print("Major: ");
    dataFile.println(major);

    dataFile.print("Minor: ");
    dataFile.println(minor);

    dataFile.print("Distance: ");
    dataFile.print(distance);
    dataFile.println(" m");

    dataFile.print("RSSI: ");
    dataFile.println(rssi);

    dataFile.print("Manufacturer: ");
    dataFile.println(manufacturerName);

    dataFile.print("Manufacturer ID: 0x");
    dataFile.println(manufacturerId, HEX);

    dataFile.println("-------------------------------");

    dataFile.flush();
}


