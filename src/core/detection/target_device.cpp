#include "target_device.h"
#include "config/detection_config.h"
#include "infrastructure/logging/logger.h"

bool isTargetDevice(String name, String address, String serviceUuid, String deviceInfoService) {

  // 0. CATHACK / BRUCE Signatur
  if ((name == "esp32" || name == "ESP32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0" || name == "Keyboard_e9" || name == "BruceNet")) {
    LOG(LOG_TARGET, "🎯 Device with ESP32 Hardware detected");
    return true;
  }

  if ((deviceInfoService == "esp32" || deviceInfoService == "n/a" || deviceInfoService == "<no name>" || deviceInfoService == "Keyboard_a0" || deviceInfoService == "Keyboard_e9" || deviceInfoService == "BruceNet")) {
    LOG(LOG_TARGET, "🎯 Device with ESP32 Hardware detected");
    return true;
  }

  // 1. LIGHTBLUE APP UUIDs
  if (serviceUuid == LIGHTBLUE_APP_SERVICE_UUID) {
    LOG(LOG_TARGET, "🎯 Device with LIGHT BLUE APP detected");
    return true;
  }

  // 2. CATHACK-Signatur
  if (serviceUuid == CATHACK_SERVICE_UUID_3 &&
      (name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0")) {
    LOG(LOG_TARGET, "🎯 Device with CATHACK Firmware detected");
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
      LOG(LOG_TARGET, "🐬 FLIPPER ZERO detected (Black)");
      return true;
  }
  if (uuid == "0x3082") {
      LOG(LOG_TARGET, "🐬 FLIPPER ZERO detected (White)");
      return true;
  }
  if (uuid == "0x3083") {
      LOG(LOG_TARGET, "🐬 FLIPPER ZERO detected (Transparent)");
      return true;
  }

  // 4. PWNBEACON (PwnGrid over BLE)
  if (serviceUuid == PWNBEACON_SERVICE_UUID) {
    LOG(LOG_TARGET, "👾 PWNBEACON detected (PwnGrid/Pwnagotchi)");
    return false;
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
