#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct BLEAdvertisementData {
    // Basic Informations
    std::string mac;
    std::string name;

    // Data from advertisement
    std::string manufacturerData;
    std::string serviceData;

    // Many UUIDs possible (z.B. iBeacon + Eddystone + Custom Service)
    std::vector<std::string> serviceUUIDs;

    // Flags / Capabilities
    bool isConnectable = false;

    // Signal
    int rssi = 0;

    // Optional for future adaption
    uint16_t appearance = 0;
};