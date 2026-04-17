#pragma once
#include <functional>
#include "soft_fingerprint.h"

struct SoftFingerprintHash {
    std::size_t operator()(const SoftFingerprint& fp) const {
        return std::hash<uint32_t>()(fp.manufacturerHash) ^
               (std::hash<uint32_t>()(fp.serviceHash) << 1) ^
               (std::hash<uint16_t>()(fp.appearance) << 2);
    }
};
