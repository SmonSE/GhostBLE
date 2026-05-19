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


// ===== API =====
void hardwareBegin();
void hardwareUpdate();

const char* hardwareModelName();
const char* hardwareMCU();
