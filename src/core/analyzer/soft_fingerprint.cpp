#include "soft_fingerprint.h"
#include <NimBLEDevice.h>

uint32_t simpleHash(const std::string& data) {
    uint32_t hash = 5381;
    for (auto c : data) {
        hash = ((hash << 5) + hash) + c; // djb2
    }
    return hash;
}

SoftFingerprint createFingerprint(const NimBLEAdvertisedDevice* device) {
    SoftFingerprint fp;

    // Manufacturer Data
    if (device->haveManufacturerData()) {
        fp.manufacturerHash = simpleHash(device->getManufacturerData());
    }

    // Service UUIDs
    std::string combined;
    int count = device->getServiceUUIDCount();
    for (int i = 0; i < count; i++) {
        combined += device->getServiceUUID(i).toString();
    }
    if (!combined.empty()) {
        fp.serviceHash = simpleHash(combined);
    }

    // Appearance (optional, often 0)
    if (device->haveAppearance()) {
        fp.appearance = device->getAppearance();
    }

    return fp;
}

SoftFingerprint createAppleFingerprint(
    const NimBLEAdvertisedDevice* device,
    const String& localName,
    int rssi ) {
        
    SoftFingerprint fp;

    fp.manufacturerHash = 0x004C;

    if (!localName.isEmpty()) {
        std::string name = localName.c_str();
        fp.serviceHash = simpleHash(name);
    }

    int bucket = (rssi / 10) * 10;  // e.g. -67 → -60
    fp.appearance = (uint16_t)(bucket + 100);

    return fp;
}
