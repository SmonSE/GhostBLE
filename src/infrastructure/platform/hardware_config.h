#pragma once

// ===== Platform Check =====
#if !defined(CARDPUTER) && !defined(M5STICKCPLUS2) && !defined(M5STICKS3)
  #error "No hardware platform defined"
#endif

// ===== SD Card =====
#if defined(CARDPUTER)
  #define SD_CS_PIN 12
#endif

// ===== GPS Pins =====
#if defined(CARDPUTER)
  #define GPS_GROVE_RX 1
  #define GPS_GROVE_TX 2
  #define GPS_LORA_RX 15
  #define GPS_LORA_TX 13
  #define LORA_CS_PIN 5
#elif defined(M5STICKCPLUS2)
  #define GPS_GROVE_RX 33
  #define GPS_GROVE_TX 32
#elif defined(M5STICKS3)
  #define GPS_GROVE_RX 1
  #define GPS_GROVE_TX 2
#endif

#define GPS_BAUD_RATE 115200