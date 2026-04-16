#pragma once
#include <string>

enum class ExposureTier {
    None,
    Passive,
    Active,
    Consent
};

inline const char* tierToString(ExposureTier tier)
{
    switch(tier)
    {
        case ExposureTier::Passive: return "PASSIVE";
        case ExposureTier::Active:  return "ACTIVE";
        case ExposureTier::Consent: return "CONSENT";
        default: return "NONE";
    }
}

struct DeviceInfo {
    std::string mac;
    std::string name;
    std::string manufacturer;

    bool gattHasEnvironmentName = false;
    bool gattHasModelInfo = false;
    bool gattHasIdentityInfo = false;

    bool advHasName = false;                // name visible without connection
    bool gattHasName = false;               // name discovered via GATT
    bool gattHasPersonalName = false;       // e.g. "A14 von Jochen"
    bool gattHasNameIdentityData = false;   // e.g. Device Information Service with unique serial number

    bool isConnectable = false;

    bool isPublicMac = false;
    bool hasStaticMac = false;
    bool hasRotatingMac = false;

    bool hasName = false;
    bool hasManufacturerData = false;
    bool hasCleartextData = false;

    // Security analysis flags
    bool hasWritableChars = false;
    bool hasDFUService = false;
    bool hasUARTService = false;
    bool connectionEncrypted = false;
    bool hasSensitiveUnencrypted = false;
    bool supportsBrEdr = false;
    int writableCharCount = 0;
    std::string deviceFingerprint;
};
