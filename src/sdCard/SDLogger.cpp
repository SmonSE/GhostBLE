#pragma once
#include "SDLogger.h"
#include <string>
#include <vector>
#include "../analyzer/ExposureAnalyzer.h"
#include "../helper/ServiceHelper.h"  // Assuming you still need service name mapping


static const char* tierToString(ExposureTier tier)
{
    switch(tier)
    {
        case ExposureTier::Passive: return "PASSIVE";
        case ExposureTier::Active:  return "ACTIVE";
        case ExposureTier::Consent: return "CONSENT";
        default: return "NONE";
    }
}

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

void SDLogger::writeCategory(const String& category)
{
    if (!initialized || !dataFile) return;
    
    dataFile.println(category);
}

void SDLogger::writeDeviceInfo( const String& address, 
                                const String& localName,
                                const std::vector<std::string>& nameList,
                                const String& manuInfo,
                                const String& deviceInfoString) 
{
    if (!initialized || !dataFile) return;
    
    dataFile.println("\n--------- New Target ----------");
    dataFile.println("\nDevice Info");
    dataFile.println("   Address: " + address);

    if (localName.length() > 0) {
        dataFile.println("   Local Name: " + localName);
    } else {
        dataFile.println("   Local Name: (no name)");
    }

    //Device Info String: 
    dataFile.println(deviceInfoString);
    
    dataFile.println(manuInfo);

    dataFile.println("   Characteristic Name:");
    for (const auto& names : nameList) {
        if (!names.empty()) {
            dataFile.print("   - ");
            dataFile.println(names.c_str());
        }
    }

    dataFile.flush();  // Make sure the data is written to the card
}

void SDLogger::writeUncovered(    const ExposureResult& exposure) 
{
    if (!initialized || !dataFile) return;

    dataFile.println("\nUncovering Summary");
    dataFile.println("   Device Type: " + String(exposure.deviceType.c_str()));
    dataFile.println("   Identity Uncovering: " + String(exposure.identityExposure.c_str()));
    dataFile.println("   Tracking Risk: " + String(exposure.trackingRisk.c_str()));
    dataFile.println("   Privacy Level: " + String(exposure.privacyLevel.c_str()));
    dataFile.println("   Uncovering Tier: " + String(tierToString(exposure.exposureTier)));

    dataFile.println("\n   Reason");
    for (auto& r : exposure.reasons) {
        dataFile.println("   - " + String(r.c_str()));
    }

    dataFile.println("\n-------------------------------");
    dataFile.flush();  // Make sure the data is written to the card
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

    dataFile.println("\n----------- iBeacon -----------");

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


