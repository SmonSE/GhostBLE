#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <map>
#include "core/models/device_info.h"

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

// isPublicAddrType comes from the BLE stack's own address-type report
// (e.g. advertisedDevice->getAddress().getType() == BLE_ADDR_PUBLIC),
// NOT inferred from the MAC bytes — Public vs Random can't be derived
// from the address itself.
MACType getMACType(const std::string& mac, bool isPublicAddrType);
String macTypeToString(MACType type);
bool isRotatingMAC(MACType type);

// === Functions ===

// MAC privacy
bool isUniversallyAdministeredMAC(const std::string& mac);

bool containsCleartext(const std::vector<uint8_t>& payload);

// Privacy analysis
// isPublicAddrType: pass the advertisement's actual reported address
// type (Public vs Random) from the BLE stack — see getMACType() above.
void handleDevicePrivacy(const std::string& name, const std::string& mac, const std::string& adv_data, const std::vector<uint8_t>& payloadVec, bool is_connectable, bool isPublicAddrType, DeviceInfo& dev, const String& devTag = "");

// Fingerprinting
std::string getIdentityFingerprint(const std::string& name, const std::string& adv_data);

// Cleartext check
bool isLikelyCleartextBytes(const std::vector<uint8_t>& bytes, size_t minLength = 6);

// Utility
String payloadToHexString(const String& payload);
