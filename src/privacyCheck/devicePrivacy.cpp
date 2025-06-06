#include "devicePrivacy.h"
#include "../globals/globals.h"

#include <map>
#include <vector>
#include <algorithm>

std::map<std::string, DeviceInfo> mac_history;
std::map<std::string, DevicePrivacyInfo> device_identity_history;
std::vector<std::string> weakNames = {"<NoName>", "BLE_Device", "Unknown", "SensorTag", "ESP32"};

bool hasWeakName(const std::string& name) {
    return std::find(weakNames.begin(), weakNames.end(), name) != weakNames.end();
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

bool isRandomMAC(const std::string& mac) {
    uint8_t firstByte = std::stoi(mac.substr(0, 2), nullptr, 16);
    return (firstByte & 0xC0) == 0x40; // 0b01xxxxxx → Random Static
}

bool isStaticPublicMAC(const std::string& mac) {
    uint8_t firstByte = std::stoi(mac.substr(0, 2), nullptr, 16);
    return (firstByte & 0xC0) == 0x00; // 0b00xxxxxx → public MAC
}

String getMACPrivacyLabel(const std::string& mac) {
    uint8_t firstByte = std::stoi(mac.substr(0, 2), nullptr, 16);

    if ((firstByte & 0xC0) == 0x00) {
      riskScore += 3;
      return "Public (trackable)";
    } else if ((firstByte & 0xC0) == 0x40) {
      riskScore += 1;
      return "Static Random (semi-private)";
    } else if ((firstByte & 0xC0) == 0x80) {
      return "Resolvable Private (private)";
    } else if ((firstByte & 0xC0) == 0xC0) {
      return "Non-resolvable Private (private)";
    } else {
      return "Unknown";
    }                               
}

void handleDevicePrivacy(const std::string& name, const std::string& mac, const std::string& adv_data, const std::vector<uint8_t>& payloadVec, bool is_connectable) {
  std::string identityKey = getIdentityFingerprint(name, adv_data);
  auto& info = device_identity_history[identityKey];

  if (!info.seen_macs.empty() && std::find(info.seen_macs.begin(), info.seen_macs.end(), mac) == info.seen_macs.end()) {
    info.mac_change_count++;
    info.seen_macs.push_back(mac);
  } else if (info.seen_macs.empty()) {
    info.seen_macs.push_back(mac);
  }

  bool weakName = hasWeakName(name.c_str());
  bool rotating_mac = isRandomMAC(mac);
  bool staticPublic_mac = isStaticPublicMAC(mac);
  bool adv_contains_cleartext = adv_data.find("http") != std::string::npos || isLikelyCleartextBytes(payloadVec);
  String macPrivacyLavel = getMACPrivacyLabel(mac);

  String logLineWebSocket = "🔍 " + String(name.c_str()) + " MAC: " + String(mac.c_str()) + "\n" +
                  "   Has rotating MAC: " + (rotating_mac ? "YES" : "NO") + "\n" +
                  "   Has privacy:      " + macPrivacyLavel + "\n" +
                  "   Has weak name:    " + (weakName ? "YES" : "NO") + "\n" +
                  "   Has cleartext:    " + (adv_contains_cleartext ? "YES" : "NO") + "\n" +
                  "   Is connectable:   " + (is_connectable ? "NO" : "YES");

  logToSerialAndWeb(logLineWebSocket);

  // Risk Score
  //if (staticPublic_mac) riskScore += 3; // added at getMACPrivacyLabel
  if (weakName) riskScore += 3;
  if (rotating_mac) riskScore -= 1;
  if (!rotating_mac) riskScore += 1;
  if (!is_connectable) riskScore += 2;
  if (is_connectable) riskScore -= 2;
  if (adv_contains_cleartext) riskScore += 2;
  //if (usesVulnerableUUIDs) riskScore += 2;  // added below 

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
