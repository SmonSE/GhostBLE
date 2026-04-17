#pragma once
#include <cstdint>
#include <NimBLEDevice.h>

struct SoftFingerprint {
    uint32_t manufacturerHash = 0;
    uint32_t serviceHash = 0;
    uint16_t appearance = 0;

    bool operator==(const SoftFingerprint& other) const {
        return manufacturerHash == other.manufacturerHash &&
               serviceHash == other.serviceHash &&
               appearance == other.appearance;
    }
};

SoftFingerprint createFingerprint(const NimBLEAdvertisedDevice* device);