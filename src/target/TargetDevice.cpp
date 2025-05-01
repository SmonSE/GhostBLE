#include "targetDevice.h"
#include "../config/config.h"

bool isTargetDevice(String name, String address, String serviceUuid, bool hasManuData) {
  // 0. CATHACK-Signatur
  if ((name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0" || name == "Keyboard_a9")) {
    Serial.println("🎯 Device with ESP32 Hardware detected");
    return true;
  }

  // 1. LIGHTBLUE APP UUIDs
  if (serviceUuid == LIGHTBLUE_APP_SERVICE_UUID) {
    Serial.println("🎯 Device with LIGHT BLUE APP detected");
    return true;
  }

  // 2. CATHACK-Signatur
  if ((serviceUuid == CATHACK_SERVICE_UUID_5 ||
       serviceUuid == CATHACK_SERVICE_UUID_6) &&
      (name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0")) {
    Serial.println("🎯 Device with CATHACK Firmware detected");
    return true;
  }

  // 3. FLIPPER ZERO UUIDs
  if (serviceUuid == FLIPPER_BLACK_UUID) {
    Serial.println("🐬 FLIPPER ZERO detected (Black)");
    return true;
  }
  if (serviceUuid == FLIPPER_WHITE_UUID) {
    Serial.println("🐬 FLIPPER ZERO detected (White)");
    return true;
  }
  if (serviceUuid == FLIPPER_TRANSPARENT_UUID) {
    Serial.println("🐬 FLIPPER ZERO detected (Transparent)");
    return true;
  }

  return false;
}
