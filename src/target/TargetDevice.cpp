#include "targetDevice.h"
#include "../config/config.h"
#include "../logger/logger.h"

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
  if (serviceUuid == FLIPPER_BLACK_UUID) {
    LOG(LOG_TARGET, "🐬 FLIPPER ZERO detected (Black)");
    return true;
  }
  if (serviceUuid == FLIPPER_WHITE_UUID) {
    LOG(LOG_TARGET, "🐬 FLIPPER ZERO detected (White)");
    return true;
  }
  if (serviceUuid == FLIPPER_TRANSPARENT_UUID) {
    LOG(LOG_TARGET, "🐬 FLIPPER ZERO detected (Transparent)");
    return true;
  }

  // 4. PWNBEACON (PwnGrid over BLE)
  if (serviceUuid == PWNBEACON_SERVICE_UUID) {
    LOG(LOG_TARGET, "👾 PWNBEACON detected (PwnGrid/Pwnagotchi)");
    return true;
  }

  return false;
}
