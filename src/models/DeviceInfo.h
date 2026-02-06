#pragma once
#include <string>

enum class ExposureTier {
    None,
    Passive,
    Active,
    Consent
};

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
};
