#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>
#include "../models/DeviceInfo.h"

// === Structs ===
struct DevicePrivacyInfo {
    std::vector<std::string> seen_macs;
    int mac_change_count = 0;
};

enum class MACType {
    Public,
    StaticRandom,
    ResolvablePrivate,
    NonResolvablePrivate,
    Unknown
};

MACType getMACType(const std::string& mac);
String macTypeToString(MACType type);
bool isRotatingMAC(MACType type);

// === Functions ===

// MAC privacy
bool isUniversallyAdministeredMAC(const std::string& mac);

bool containsCleartext(const std::vector<uint8_t>& payload);

// Privacy analysis
void handleDevicePrivacy(const std::string& name, const std::string& mac, const std::string& adv_data, const std::vector<uint8_t>& payloadVec, bool is_connectable, DeviceInfo& dev);

// Fingerprinting
std::string getIdentityFingerprint(const std::string& name, const std::string& adv_data);

// Cleartext check
bool isLikelyCleartextBytes(const std::vector<uint8_t>& bytes, size_t minLength = 6);

// Utility
String payloadToHexString(const String& payload);
