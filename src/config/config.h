#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== Hardware Platform =====
//#define STICK_C_PLUS2
#define CARDPUTER

// ===== SD Card Pin =====
#if defined(CARDPUTER)
  #define SD_CS_PIN 12
#elif defined(STICK_C_PLUS2)
  #define SD_CS_PIN 14
#else
  #error "No hardware platform defined. Define CARDPUTER or STICK_C_PLUS2."
#endif

// ===== Flipper Zero UUIDs =====
extern const char* FLIPPER_BLACK_UUID;
extern const char* FLIPPER_WHITE_UUID;
extern const char* FLIPPER_TRANSPARENT_UUID;

// ===== LightBlue App UUID =====
extern const char* LIGHTBLUE_APP_SERVICE_UUID;

// ===== CATHACK Apple Juice UUIDs =====
extern const char* CATHACK_SERVICE_UUID_0;
extern const char* CATHACK_SERVICE_UUID_1;
extern const char* CATHACK_SERVICE_UUID_2;
extern const char* CATHACK_SERVICE_UUID_3;
extern const char* CATHACK_SERVICE_UUID_4;
extern const char* CATHACK_SERVICE_UUID_5;
extern const char* CATHACK_SERVICE_UUID_6;

// ===== WiFi AP Settings =====
#define WIFI_AP_SSID "GhostBLE"
#define WIFI_AP_PASSWORD "ghostble123!"

// ===== Time Intervals =====
#define FACE_UPDATE_INTERVAL_MS 1000

// ===== Constants =====
#define RSSI_CONSTANT 20
#define DISTANCE_CONSTANT -69

#endif // CONFIG_H