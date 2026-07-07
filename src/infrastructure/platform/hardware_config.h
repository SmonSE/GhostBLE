#pragma once

// ===== Platform Check =====
#if !defined(CARDPUTER) && !defined(M5STICKCPLUS2) && !defined(M5STICKS3)
  #error "No hardware platform defined"
#endif

// ===== Feature flags =====
#if defined(CARDPUTER)
  #define HAS_KEYBOARD  1
  #define HAS_SD_CARD   1
  #define HAS_SPEAKER   1
  #define HAS_TWO_BUTTONS 0
  #define SD_CS_PIN     12
#elif defined(M5STICKS3)
  #define HAS_KEYBOARD    0
  #define HAS_SD_CARD     1        // ← neu: externe SD vorhanden
  #define HAS_SPEAKER     1
  #define HAS_TWO_BUTTONS 1
  // Externe SD-Karte via SPI (manuell verdrahtet)
  #define SD_CS_PIN       8        // G8
  #define SD_MOSI_PIN     1        // G1
  #define SD_CLK_PIN      0        // G0
  #define SD_MISO_PIN     4        // G4
#elif defined(M5STICKCPLUS2)
  #define HAS_KEYBOARD  0
  #define HAS_SD_CARD   0
  #define HAS_SPEAKER   1
  #define HAS_TWO_BUTTONS 1
  #define SD_CS_PIN     -1
#endif

// ===== GPS Pins =====
#if defined(CARDPUTER)
  #define GPS_GROVE_RX  1
  #define GPS_GROVE_TX  2
  #define GPS_LORA_RX   15
  #define GPS_LORA_TX   13
  #define LORA_CS_PIN   5
#elif defined(M5STICKCPLUS2)
  #define GPS_GROVE_RX  33
  #define GPS_GROVE_TX  32
#elif defined(M5STICKS3)
  #define GPS_GROVE_RX  1
  #define GPS_GROVE_TX  2
#endif

#define GPS_BAUD_RATE 115200