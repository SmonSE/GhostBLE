#include "hardware.h"

// ===== Device initialization =====
void hardwareBegin() {
#if defined(CARDPUTER)
    M5.Power.begin();
    M5Cardputer.begin();
#elif defined(M5STICKCPLUS2) || defined(M5STICKS3)
    auto cfg = M5.config();
    M5.begin(cfg);
#endif
}

// ===== Device update =====
void hardwareUpdate() {
#if defined(CARDPUTER)
    M5Cardputer.update();
#else
    M5.update();
#endif
}

// ===== Device model string =====
const char* hardwareModelName() {
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

// ===== MCU type =====
const char* hardwareMCU() {
#if defined(M5STICKCPLUS2)
    return "ESP32";
#else
    return "ESP32-S3";
#endif
}