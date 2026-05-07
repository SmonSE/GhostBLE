#pragma once

#include <Arduino.h>
#include <functional>

// ===== Kategorien =====

enum SdoCategory {
    SDO_CAT_UNKNOWN,
    SDO_CAT_DRONE,
    SDO_CAT_SECURITY,
    SDO_CAT_IOT,
    SDO_CAT_MISC
};

// ===== Threat Level =====

enum SdoThreatLevel {
    SDO_THREAT_LOW,
    SDO_THREAT_MEDIUM,
    SDO_THREAT_HIGH
};

// ===== Result Struct =====

struct SdoResult {
    uint16_t uuid;
    const char* name;
    SdoCategory category;
    SdoThreatLevel threat;
};

// ===== Handler Typ =====

using SdoHandler = std::function<void()>;

// ===== Parser =====

class SdoServiceParser {
public:
    static bool parse(uint16_t uuid, SdoResult& result);
};
