#pragma once
#include <string>
#include <unordered_set>
#include "soft_fingerprint_hash.h"


class DeviceRegistry {
public:

    bool isNewDevice(const std::string& addr);
    bool isNewFingerprint(const SoftFingerprint& fp);

    size_t size() const;
    void clear();

private:
    std::unordered_set<std::string> seenDevices;
    std::unordered_set<SoftFingerprint, SoftFingerprintHash> seenFingerprints;
};