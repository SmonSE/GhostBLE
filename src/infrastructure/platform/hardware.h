#pragma once

#include <M5Unified.h>

// ===== Hardware Platform Includes =====
#if defined(CARDPUTER)
  #include <M5Cardputer.h>
#elif defined(M5STICKCPLUS2)
  #include <M5StickCPlus2.h>
#elif defined(M5STICKS3)
  // handled by M5Unified
#else
  #error "No hardware platform defined. Set build flag: CARDPUTER, M5STICKCPLUS2, or M5STICKS3"
#endif

// ===== Platform feature flags =====
#if defined(CARDPUTER)
  #define HAS_KEYBOARD   1
  #define HAS_SD_CARD    1
#else
  #define HAS_KEYBOARD   0
  #define HAS_SD_CARD    0
#endif

#if defined(M5STICKCPLUS2) || defined(M5STICKS3)
  #define HAS_TWO_BUTTONS 1
#else
  #define HAS_TWO_BUTTONS 0
#endif

// ===== API =====
void hardwareBegin();
void hardwareUpdate();

const char* hardwareModelName();
const char* hardwareMCU();