#ifndef DEVICE_PRIVACY_H
#define DEVICE_PRIVACY_H

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
bool isRandomMAC(const std::string& mac);
bool isUniversallyAdministeredMAC(const std::string& mac);
String getMACPrivacyLabel(const std::string& mac);

bool containsCleartext(const std::vector<uint8_t>& payload);

// Privacy analysis
void handleDevicePrivacy(const std::string& name, const std::string& mac, const std::string& adv_data, const std::vector<uint8_t>& payloadVec, bool is_connectable, DeviceInfo& dev);

// Fingerprinting
std::string getIdentityFingerprint(const std::string& name, const std::string& adv_data);

// Cleartext check
bool isLikelyCleartextBytes(const std::vector<uint8_t>& bytes, size_t minLength = 6);

// Utility
String payloadToHexString(const String& payload);

// Logging (defined elsewhere)
void logToSerialAndWeb(const String& line);

#endif // DEVICE_PRIVACY_H
