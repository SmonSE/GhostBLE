#ifndef DEVICE_PRIVACY_H
#define DEVICE_PRIVACY_H

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>

// === Structs ===

struct DeviceInfo {
    std::string last_mac;
    uint8_t mac_change_count = 0;
    std::vector<std::string> seen_macs;
};

struct DevicePrivacyInfo {
    std::vector<std::string> seen_macs;
    int mac_change_count = 0;
};

// === Functions ===

// MAC privacy
bool isRandomMAC(const std::string& mac);
bool isStaticPublicMAC(const std::string& mac);
String getMACPrivacyLabel(const std::string& mac);

// Privacy analysis
void handleDevicePrivacy(const std::string& name, const std::string& mac, const std::string& adv_data, const std::vector<uint8_t>& payloadVec, bool is_connectable);

// Fingerprinting
std::string getIdentityFingerprint(const std::string& name, const std::string& adv_data);

// Cleartext check
bool isLikelyCleartextBytes(const std::vector<uint8_t>& bytes, size_t minLength = 6);

// Utility
String payloadToHexString(const String& payload);

// Logging (defined elsewhere)
void logToSerialAndWeb(const String& line);

#endif // DEVICE_PRIVACY_H
