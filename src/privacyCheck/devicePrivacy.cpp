#include "devicePrivacy.h"
#include "../globals/globals.h"
#include "../sdCard/SDLogger.h"

extern SDLogger sdLogger; // -> not nice impl, but for logging in this file we need access to the SDLogger instance

#include <map>
#include <vector>
#include <algorithm>

std::map<std::string, DeviceInfo> mac_history;
std::map<std::string, DevicePrivacyInfo> device_identity_history;
std::vector<std::string> weakNames = {"<NoName>", "BLE_Device", "Unknown", "SensorTag", "ESP32"};
std::vector<std::string> emptyNames = {"", "< -- >"};

enum class DeviceCategory {
    LOW_RISK,
    UNCOVERING,
    MISCONFIGURATION,
    POTENTIAL_VULNERABILITY
};

MACType getMACType(const std::string& mac)
{
    if (mac.length() < 2) return MACType::Unknown;

    uint8_t firstByte = std::stoi(mac.substr(0,2), nullptr, 16);

    uint8_t typeBits = firstByte & 0xC0;

    switch(typeBits)
    {
        case 0x00:
            return MACType::Public;

        case 0x40:
            return MACType::StaticRandom;

        case 0x80:
            return MACType::ResolvablePrivate;

        case 0xC0:
            return MACType::NonResolvablePrivate;

        default:
            return MACType::Unknown;
    }
}

String macTypeToString(MACType type)
{
    switch(type)
    {
        case MACType::Public:
            return "Public";

        case MACType::StaticRandom:
            return "Static Random (semi-private)";

        case MACType::ResolvablePrivate:
            return "Resolvable Private (private)";

        case MACType::NonResolvablePrivate:
            return "Non-Resolvable Private (very private)";

        default:
            return "Unknown";
    }
}

bool isRotatingMAC(MACType type)
{
    return (type == MACType::ResolvablePrivate ||
            type == MACType::NonResolvablePrivate);
}

bool hasWeakName(const std::string& name) {
    return std::find(weakNames.begin(), weakNames.end(), name) != weakNames.end();
}

bool hasEmptyName(const std::string& name) {
    return std::find(emptyNames.begin(), emptyNames.end(), name) != emptyNames.end();
}

// Funktion um Geräte über z.B. Services zu identifizieren
std::string getIdentityFingerprint(const std::string& name, const std::string& adv_data) {
    return name + "|" + adv_data;  // simple fingerprint
}

bool isLikelyCleartextBytes(const std::vector<uint8_t>& bytes, size_t minLength) {
  size_t printableCount = 0;
  for (uint8_t b : bytes) {
    if (b >= 32 && b <= 126) {
      printableCount++;
    }
  }
  return printableCount >= minLength;
}

#include "devicePrivacy.h"

bool isStaticPublicMAC(const std::string& mac)
{
    // einfache Heuristik:
    // Public MAC = kein Random Bit gesetzt
    // (für GhostBLE erstmal ausreichend)

    if (mac.length() < 2)
        return false;

    // erstes Byte der MAC
    unsigned int firstByte = std::stoi(mac.substr(0, 2), nullptr, 16);

    // Bit 1 = locally administered
    bool locallyAdministered = firstByte & 0x02;

    // wenn nicht locally administered → public MAC
    return !locallyAdministered;
}

bool containsCleartext(const std::vector<uint8_t>& payload)
{
    for (auto b : payload) {
        if (b >= 32 && b <= 126) {
            return true;
        }
    }
    return false;
}

DeviceCategory classifyDevice(
    bool weakName,
    bool emptyName,
    bool rotating_mac,
    bool staticPublic_mac,
    bool adv_contains_cleartext,
    bool is_connectable)
{
    // Potential vulnerability: static MAC + cleartext data exposed
    if (!rotating_mac && adv_contains_cleartext && staticPublic_mac) {
        return DeviceCategory::POTENTIAL_VULNERABILITY;
    }

    // Uncovering: device exposes identity through multiple signals
    if ((!emptyName && adv_contains_cleartext) || (staticPublic_mac && adv_contains_cleartext)) {
        return DeviceCategory::UNCOVERING;
    }

    // Misconfiguration: weak name combined with open connectivity
    if (weakName && is_connectable) {
        return DeviceCategory::MISCONFIGURATION;
    }

    return DeviceCategory::LOW_RISK;
}

String categoryToString(DeviceCategory cat)
{
    switch (cat) {
        case DeviceCategory::UNCOVERING:
            return "Uncovering";
        case DeviceCategory::MISCONFIGURATION:
            return "Misconfiguration";
        case DeviceCategory::POTENTIAL_VULNERABILITY:
            return "Potential Vulnerability";
        default:
            return "Low Risk";
    }
}
void handleDevicePrivacy(
    const std::string& name,
    const std::string& mac,
    const std::string& adv_data,
    const std::vector<uint8_t>& payloadVec,
    bool is_connectable,
    DeviceInfo& dev)
{
    std::string identityKey = getIdentityFingerprint(name, adv_data);
    auto& info = device_identity_history[identityKey];

    if (!info.seen_macs.empty() &&
        std::find(info.seen_macs.begin(), info.seen_macs.end(), mac) == info.seen_macs.end()) {
        info.mac_change_count++;
        info.seen_macs.push_back(mac);
    } else if (info.seen_macs.empty()) {
        info.seen_macs.push_back(mac);
    }

    if (name.find("von ") != std::string::npos){
      dev.gattHasPersonalName = true;
    }

    if (adv_data.find("Device Information") != std::string::npos &&
        adv_data.find("Serial Number") != std::string::npos) {
        dev.gattHasNameIdentityData = true;
    }
      
    bool weakName = hasWeakName(name);
    bool emptyName = hasEmptyName(name);
    bool adv_contains_cleartext =
        adv_data.find("http") != std::string::npos ||
        isLikelyCleartextBytes(payloadVec);

    // ✅ SINGLE SOURCE OF TRUTH
    MACType macType = getMACType(mac);
    bool rotating_mac = isRotatingMAC(macType);
    String macPrivacyLabel = macTypeToString(macType);

    bool staticPublic_mac = (macType == MACType::Public);

    bool isLikelyConsumerDevice =
        name.find("JBL") != std::string::npos ||
        name.find("Sony") != std::string::npos ||
        name.find("Bose") != std::string::npos ||
        name.find("AirPods") != std::string::npos;

    if (isLikelyConsumerDevice && rotating_mac) {
        riskScore -= 3;
    }

    DeviceCategory category = classifyDevice(
        weakName,
        emptyName,
        rotating_mac,
        staticPublic_mac,
        adv_contains_cleartext,
        is_connectable
    );

    String categoryStr = categoryToString(category);

    String logLineWebSocket =
        "\nName: " + String(name.c_str()) + " MAC: " + String(mac.c_str()) + "\n" +
        "   Category:          " + categoryStr + "\n" +
        "   MAC Type:          " + macPrivacyLabel + "\n" +
        "   Has rotating MAC: " + (rotating_mac ? " YES" : " NO") + "\n" +
        "   Empty name:       " + (emptyName ? " YES" : " NO") + "\n" +
        "   Weak name:        " + (weakName ? " YES" : " NO") + "\n" +
        "   Cleartext data:   " + (adv_contains_cleartext ? " YES" : " NO") + "\n" +
        "   Connectable:      " + (is_connectable ? " YES" : " NO");

    logToSerialAndWeb(logLineWebSocket);
    //sdLogger.writeCategory(logLineWebSocket);
    
    // ---- Risk score ----
    if (weakName) riskScore += 3;
    if (!emptyName && !rotating_mac) riskScore += 3;
    if (rotating_mac) riskScore -= 1;
    else riskScore += 1;

    if (!is_connectable) riskScore += 2;
    else riskScore -= 2;

    if (adv_contains_cleartext) riskScore += 2;
}

// Helper function to convert raw payload bytes to hex string
String payloadToHexString(const String& payload) {
    String hexStr = "";
    for (size_t i = 0; i < payload.length(); i++) {
        uint8_t b = payload.charAt(i);
        if (b < 0x10) hexStr += "0";
        hexStr += String(b, HEX);
        hexStr += " ";
    }
    return hexStr;
}
// PRIVACY NEW
