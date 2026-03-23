#pragma once

// ===== Hardware Platform Includes =====
// Include the correct device library based on build flags.
// Build flags (-DCARDPUTER, -DM5STICKCPLUS2, -DM5STICKS3) are set in platformio.ini.

#if defined(CARDPUTER)
  #include <M5Cardputer.h>
#elif defined(M5STICKCPLUS2)
  #include <M5StickCPlus2.h>
#elif defined(M5STICKS3)
  // M5StickS3 is supported via M5Unified
#else
  #error "No hardware platform defined. Set build flag: CARDPUTER, M5STICKCPLUS2, or M5STICKS3"
#endif

#include <M5Unified.h>

// ===== Platform feature flags =====
#if defined(CARDPUTER)
  #define HAS_KEYBOARD   1
  #define HAS_SD_CARD    1
#else
  #define HAS_KEYBOARD   0
  #define HAS_SD_CARD    0
#endif

// Two-button devices (StickC Plus 2, StickS3)
#if defined(M5STICKCPLUS2) || defined(M5STICKS3)
  #define HAS_TWO_BUTTONS 1
#else
  #define HAS_TWO_BUTTONS 0
#endif

// ===== Device initialization =====
inline void hardwareBegin() {
#if defined(CARDPUTER)
  M5.Power.begin();
  M5Cardputer.begin();
#elif defined(M5STICKCPLUS2) || defined(M5STICKS3)
  auto cfg = M5.config();
  M5.begin(cfg);
#endif
}

// ===== Device update (call in loop) =====
inline void hardwareUpdate() {
#if defined(CARDPUTER)
  M5Cardputer.update();
#else
  M5.update();
#endif
}

// ===== Device model string (for WiGLE, logging) =====
inline const char* hardwareModelName() {
#if defined(CARDPUTER)
  return "M5Cardputer";
#elif defined(M5STICKCPLUS2)
  return "M5StickCPlus2";
#elif defined(M5STICKS3)
  return "M5StickS3";
#else
  return "Unknown";
#endif
}

inline const char* hardwareMCU() {
#if defined(M5STICKCPLUS2)
  return "ESP32";
#else
  return "ESP32-S3";
#endif
}
