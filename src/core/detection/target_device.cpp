#include "target_device.h"
#include "config/detection_config.h"
#include "infrastructure/logging/logger.h"

bool isTargetDevice(String name, String address, String serviceUuid, String deviceInfoService, String& outLabel) {

  // 0. CATHACK / BRUCE Signatur
  if ((name == "esp32" || name == "ESP32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0" || name == "Keyboard_e9" || name == "BruceNet")) {
    outLabel = "ESP32 Hardware";
    LOG(LOG_TARGET, "Device with ESP32 Hardware detected");
    return true;
  }

  if ((deviceInfoService == "esp32" || deviceInfoService == "n/a" || deviceInfoService == "<no name>" || deviceInfoService == "Keyboard_a0" || deviceInfoService == "Keyboard_e9" || deviceInfoService == "BruceNet")) {
    outLabel = "ESP32 Hardware";
    LOG(LOG_TARGET, "Device with ESP32 Hardware detected");
    return true;
  }

  // 1. LIGHTBLUE APP UUIDs
  if (serviceUuid == LIGHTBLUE_APP_SERVICE_UUID) {
    outLabel = "LIGHT BLUE APP";
    LOG(LOG_TARGET, "Device with LIGHT BLUE APP detected");
    return true;
  }

  // 2. CATHACK-Signatur
  if (serviceUuid == CATHACK_SERVICE_UUID_3 &&
      (name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0")) {
    outLabel = "CATHACK Firmware";
    LOG(LOG_TARGET, "Device with CATHACK Firmware detected");
    return true;
  }

  // 3. FLIPPER ZERO UUIDs
  String uuid = serviceUuid;
  uuid.toLowerCase();

  // Normalize to 16-bit UUID
  if (uuid.length() == 4) {
  // already short → do nothing
  } else if (uuid.length() >= 8) {
      uuid = uuid.substring(4, 8);
  }
  // Detection
  if (uuid == "0x3081") {
      outLabel = "FLIPPER ZERO (Black)";
      LOG(LOG_TARGET, "FLIPPER ZERO detected (Black)");
      return true;
  }
  if (uuid == "0x3082") {
      outLabel = "FLIPPER ZERO (White)";
      return true;
  }
  if (uuid == "0x3083") {
      outLabel = "FLIPPER ZERO (Transparent)";
      return true;
  }

  // 4. PWNBEACON (PwnGrid over BLE)
  if (serviceUuid == PWNBEACON_SERVICE_UUID) {
    outLabel = "PWNBEACON (PwnGrid/Pwnagotchi)";
    LOG(LOG_TARGET, "👾 PWNBEACON detected (PwnGrid/Pwnagotchi)");
    return false;
  }

  // 5. XIAO BISCUIT
  String uuidLower = serviceUuid;
  uuidLower.toLowerCase();
  if (uuidLower == XIAO_BISCUIT_SERVICE_UUID || uuidLower == XIAO_BISCUIT_SERVICE_UUID_2) {
    outLabel = "XIAO BISCUIT";
    LOG(LOG_TARGET, "XIAO BISCUIT detected (service UUID match)");
    return true;
  }
  if (name == "Xiao Biscuit") {
    outLabel = "XIAO BISCUIT";
    LOG(LOG_TARGET, "XIAO BISCUIT detected (name match)");
    return true;
  }

  // 6. POTENTIAL CARD SKIMMER (generic serial BLE module names)
  if (name == "HC-03" || name == "HC-05" || name == "HC-06" ||
      name == "HM-10" || name == "HM-19") {
    outLabel = "Possible Card Skimmer";
    LOG(LOG_TARGET, "Possible card skimmer module detected: " + name);
    return true;
  }

  return false;
}

// --- Tesla vehicle detection ---
static bool isHexChar(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

bool isTeslaDevice(const String& name, const String& serviceUuid) {
  // Match by Tesla BLE GATT service UUID
  if (serviceUuid == TESLA_BLE_SERVICE_UUID) {
    return true;
  }

  // Legacy name format: S + 16 hex chars + [C/D/P/R]
  // e.g. "Sc155040258896e2dC"
  if (name.length() == 18 && name[0] == 'S') {
    char suffix = name[17];
    if (suffix == 'C' || suffix == 'D' || suffix == 'P' || suffix == 'R') {
      bool allHex = true;
      for (int i = 1; i < 17; i++) {
        if (!isHexChar(name[i])) { allHex = false; break; }
      }
      if (allHex) return true;
    }
  }

  // New name format: "Tesla " + identifier
  if (name.startsWith("Tesla ") && name.length() > 6) {
    return true;
  }

  return false;
}

bool isXiaoBiscuitDevice(const String& name, const String& serviceUuid) {
  String uuidLower = serviceUuid;
  uuidLower.toLowerCase();

  if (uuidLower == XIAO_BISCUIT_SERVICE_UUID || uuidLower == XIAO_BISCUIT_SERVICE_UUID_2) {
    return true;
  }
  if (name == "Xiao Biscuit") {
    return true;
  }
  return false;
}

bool isFlipperDevice(const String& serviceUuid) {
  String uuid = serviceUuid;
  uuid.toLowerCase();

  if (uuid.length() >= 8) {
    uuid = uuid.substring(4, 8);
  }

  return (uuid == "0x3081" || uuid == "0x3082" || uuid == "0x3083");
}
