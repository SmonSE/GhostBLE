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

void SDLogger::writeDeviceInfo(const String& address, const String& localName, BLEDevice& peripheral, const std::vector<String>& serviceUuids) {
    if (!initialized) {
        Serial.println("#SDLogger# SDLogger not initialized.");
        return;
    }

    // Check if the file is open before writing
    if (dataFile) {
        dataFile.print("Device Address: ");
        dataFile.println(address);

        if (localName.length() > 0) {
            dataFile.print("Local Name: ");
            dataFile.println(localName);
        } else {
            dataFile.println("Local Name: (no name)");
        }

        int rssi = peripheral.rssi();
        dataFile.print("RSSI: ");
        dataFile.println(rssi);

        if (peripheral.hasManufacturerData()) {
            uint8_t mfgData[64];
            int mfgDataLen = peripheral.manufacturerData(mfgData, sizeof(mfgData));
            if (mfgDataLen > 0) {
                dataFile.print("Manufacturer Data: ");
                for (int i = 0; i < mfgDataLen; i++) {
                    if (mfgData[i] < 16) dataFile.print("0");  // leading zero for better display
                    dataFile.print(mfgData[i], HEX);
                }
                dataFile.println();
            }
        }

        int advServiceCount = peripheral.advertisedServiceUuidCount();
        if (advServiceCount > 0) {
            dataFile.println("Advertised Services:");
            for (int i = 0; i < advServiceCount; i++) {
                String serviceUuid = peripheral.advertisedServiceUuid(i);
                String serviceName = getServiceName(serviceUuid);
                dataFile.print("UUID: ");
                dataFile.print(serviceUuid);
                dataFile.print(" (");
                dataFile.print(serviceName);
                dataFile.println(")");
            }
        } else {
            dataFile.println("⚠ No advertised services found!");
        }

        dataFile.println("-------------------------------");
        dataFile.flush();  // Make sure the data is written to the card
    } else {
        Serial.println("#SDLogger# Error writing to file. File not open.");
    }
}
