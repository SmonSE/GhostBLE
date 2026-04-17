#pragma once

#include <string>
#include <vector>
#include <NimBLEClient.h>
#include "infrastructure/logging/logger.h"
#include "core/models/device_info.h"

struct SecurityFinding {
    std::string severity;    // "HIGH", "MEDIUM", "LOW", "INFO"
    std::string category;    // e.g. "WRITABLE_CHAR", "NO_ENCRYPTION", "DFU_EXPOSED"
    std::string description;
};

struct SecurityResult {
    bool connectionEncrypted = false;
    bool hasDFUService = false;
    bool hasUARTService = false;
    bool hasWritableWithoutAuth = false;
    bool hasSensitiveServiceUnencrypted = false;
    int writableCharCount = 0;
    int totalCharCount = 0;
    std::string deviceFingerprint;
    std::vector<SecurityFinding> findings;
};

// Analyze GATT security after connecting to a device
SecurityResult analyzeDeviceSecurity(NimBLEClient* pClient, const DeviceInfo& dev);

// Parse advertisement flags (AD Type 0x01) from raw payload
void analyzeAdvFlags(const std::vector<uint8_t>& payload, DeviceInfo& dev,
                     std::vector<SecurityFinding>& findings);

// Build a unique fingerprint from advertised service UUIDs + GATT profile
std::string buildDeviceFingerprint(NimBLEClient* pClient,
                                   const std::string& name,
                                   const std::string& manufacturer);
