#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== Hardware Platform =====
#define CARDPUTER

// ===== SD Card Pin =====
#if defined(CARDPUTER)
  #define SD_CS_PIN 12
#else
  #error "No hardware platform defined. Define CARDPUTER"
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

// ===== GPS Pins =====
// Grove port GPS (e.g. M5Stack GPS Unit on Grove UART)
#define GPS_GROVE_RX 1
#define GPS_GROVE_TX 2

// LoRa cap GPS (e.g. LoRa + GPS cap module with ATGM336H)
#define GPS_LORA_RX 15
#define GPS_LORA_TX 13

// LoRa SX1262 chip select (shared SPI bus with SD card)
#define LORA_CS_PIN 5

#define GPS_BAUD_RATE 115200

// ===== WiFi AP Settings =====
#define WIFI_AP_SSID "GhostBLE"
#define WIFI_AP_PASSWORD "ghostble123!"

// ===== Time Intervals =====
#define FACE_UPDATE_INTERVAL_MS 1000

// ===== Constants =====
#define RSSI_CONSTANT 20
#define DISTANCE_CONSTANT -69

#endif // CONFIG_H