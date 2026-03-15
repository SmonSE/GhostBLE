#pragma once

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

// ===== PwnBeacon UUIDs =====
extern const char* PWNBEACON_SERVICE_UUID;
extern const char* PWNBEACON_IDENTITY_CHAR_UUID;
extern const char* PWNBEACON_FACE_CHAR_UUID;
extern const char* PWNBEACON_NAME_CHAR_UUID;

// ===== PwnBeacon Protocol Constants =====
#define PWNBEACON_PROTOCOL_VERSION  0x01
#define PWNBEACON_ADV_MAX_NAME_LEN  8
#define PWNBEACON_FINGERPRINT_LEN   6

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

// ===== Device Name =====
#define DEVICE_NAME "NibBLEs"
#define DEVICE_FACE "(◕‿◕)"

// ===== WiFi AP Settings =====
#define WIFI_AP_SSID "GhostBLE"
#define WIFI_AP_PASSWORD "ghostble123!"

// ===== Time Intervals =====
#define FACE_UPDATE_INTERVAL_MS 1000
#define ADV_WINDOW_MS           5000   // Advertise for 5s between scan cycles

// ===== Seen Devices Limits =====
#define MAX_SEEN_DEVICES 200
#define MIN_FREE_HEAP_BYTES 20000

// ===== BLE Scan Parameters =====
#define BLE_SCAN_INTERVAL 45
#define BLE_SCAN_WINDOW   15

// ===== BLE GATT UUIDs =====
#define UUID_GENERIC_ACCESS       "1800"
#define UUID_DEVICE_NAME          "2a00"
#define UUID_MODEL_NUMBER         "2a24"
#define UUID_SERIAL_NUMBER        "2a25"
#define UUID_MANUFACTURER_NAME    "2a29"
#define UUID_BATTERY_LEVEL        "2a19"
#define UUID_HEALTH_THERMOMETER   "1809"

// ===== Constants =====
#define RSSI_CONSTANT 20
#define DISTANCE_CONSTANT -69

// ===== UI Layout Constants =====
// NibBLEs sprite positions
#define NIBBLES_FRONT_X        5
#define NIBBLES_FRONT_Y        0
#define NIBBLES_HAPPY_X        83
#define NIBBLES_HAPPY_Y        60
#define NIBBLES_SLEEP_X        83
#define NIBBLES_SLEEP_Y        60

// Speech/thought bubble geometry
#define BUBBLE_X               125
#define BUBBLE_RECT_Y          15
#define BUBBLE_RECT_W          108
#define BUBBLE_RECT_H          22
#define BUBBLE_CORNER_R        4
#define BUBBLE_TRI_OFFSET_X    8
#define BUBBLE_TRI_W           6
#define BUBBLE_TRI_H           5
#define BUBBLE_TEXT_INSET_X    6
#define BUBBLE_TEXT_INSET_Y    7
#define BUBBLE_MIN_W           60
#define BUBBLE_MAX_W           108
#define THOUGHT_BUBBLE_Y       18

// Font metrics (default font, text size 1)
#define CHAR_WIDTH_PX          6
#define BUBBLE_PADDING_PX      16

// Top status bar
#define STATUS_BAR_Y           2
#define STATUS_ICON_X          5

// Stats panel (left side)
#define STATS_X                5
#define STATS_Y_START          62
#define STATS_LINE_HEIGHT      12

// Bottom bar
#define BOTTOM_BAR_Y           127
#define LEVEL_TEXT_X           5
#define XP_BAR_X              38
#define XP_BAR_W              70
#define XP_BAR_H               7
#define TITLE_TEXT_X           112

// Border/outline color for white speech bubbles
#define BUBBLE_BORDER_COLOR    0x2104
