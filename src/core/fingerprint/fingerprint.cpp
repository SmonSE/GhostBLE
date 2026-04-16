#include "fingerprint.h"

// Simple FNV-1a Hash (schnell + stabil)
uint32_t hashString(const std::string& str)
{
    uint32_t hash = 2166136261u;

    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }

    return hash;
}

uint32_t hashCombine(uint32_t h1, uint32_t h2)
{
    // klassisches Hash Mixing
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

uint32_t makeFingerprint(const BLEAdvertisementData& data)
{
    uint32_t hash = 0;

    // --- Manufacturer Data ---
    if (!data.manufacturerData.empty()) {
        hash = hashCombine(hash, hashString(data.manufacturerData));
    }

    // --- Service Data ---
    if (!data.serviceData.empty()) {
        hash = hashCombine(hash, hashString(data.serviceData));
    }

    // --- Service UUIDs ---
    for (const auto& uuid : data.serviceUUIDs) {
        hash = hashCombine(hash, hashString(uuid));
    }

    // --- Name (optional, vorsichtig) ---
    // Nur wenn sinnvoll → sonst zu instabil
    if (!data.name.empty() && data.name.length() < 20) {
        hash = hashCombine(hash, hashString(data.name));
    }

    // --- Connectable Flag ---
    hash = hashCombine(hash, data.isConnectable ? 1 : 0);

    return hash;
}