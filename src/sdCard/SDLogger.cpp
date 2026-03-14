#pragma once
#include "SDLogger.h"
#include <string>
#include <vector>
#include "../analyzer/ExposureAnalyzer.h"
#include "../helper/ServiceHelper.h"  // Assuming you still need service name mapping
#include "../logger/logger.h"


SDLogger::SDLogger() : initialized(false) {}

SDLogger::~SDLogger() {
    end();
}

void SDLogger::end() {
    if (initialized && dataFile) {
        dataFile.flush();
        dataFile.close();
        initialized = false;
        LOG(LOG_SYSTEM, "#SDLogger# File closed.");
    }
}

void SDLogger::migrateToFolder() {
    // Migrate legacy root-level files into /GhostBLE/ folder.
    // Only called when /GhostBLE/ doesn't exist yet.
    SD.mkdir("/GhostBLE");
    LOG(LOG_SYSTEM, "#SDLogger# Created /GhostBLE folder, migrating legacy files...");

    if (SD.exists("/device_info.txt")) {
        SD.rename("/device_info.txt", "/GhostBLE/device_info.txt");
        LOG(LOG_SYSTEM, "#SDLogger# Migrated /device_info.txt");
    }

    for (int i = 1; i <= 9999; i++) {
        char oldPath[24];
        snprintf(oldPath, sizeof(oldPath), "/wigle_%04d.csv", i);
        if (!SD.exists(oldPath)) break;
        char newPath[34];
        snprintf(newPath, sizeof(newPath), "/GhostBLE/wigle_%04d.csv", i);
        SD.rename(oldPath, newPath);
        LOG(LOG_SYSTEM, "#SDLogger# Migrated " + String(oldPath));
    }

    if (SD.exists("/wigle_overflow.csv")) {
        SD.rename("/wigle_overflow.csv", "/GhostBLE/wigle_overflow.csv");
        LOG(LOG_SYSTEM, "#SDLogger# Migrated /wigle_overflow.csv");
    }
}

bool SDLogger::begin(int csPin) {
    // Initialize SD card with the correct chip select pin
    if (csPin != -1) {
        if (!SD.begin(csPin)) {
            LOG(LOG_SYSTEM, "#SDLogger# SD card initialization failed!");
            return false;
        }
    } else {
        if (!SD.begin()) {
            LOG(LOG_SYSTEM, "#SDLogger# SD card initialization failed!");
            return false;
        }
    }

    // Create folder and migrate legacy files on first run
    if (!SD.exists("/GhostBLE")) {
        migrateToFolder();
    }

    // Try opening the file with a check for success
    dataFile = SD.open("/GhostBLE/device_info.txt", FILE_APPEND);
    if (!dataFile) {
        LOG(LOG_SYSTEM, "#SDLogger# Error opening /GhostBLE/device_info.txt for writing.");
        return false;
    }

    // Log success and set initialization flag
    LOG(LOG_SYSTEM, "#SDLogger# /GhostBLE/device_info.txt opened successfully.");
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
                                const String& deviceInfoString) 
{
    if (!initialized || !dataFile) return;
    
    //dataFile.println("\n--------- New Target ----------");
    dataFile.println("\nDevice Info");
    dataFile.println("   Address: " + address);

    if (localName.length() > 0) {
        dataFile.println("   Local Name: " + localName);
    } else {
        dataFile.println("   Local Name: (no name)");
    }

    //Device Info String: 
    dataFile.println(deviceInfoString);

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

    dataFile.println("-------------------------------");

    dataFile.flush();
}


