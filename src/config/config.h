#pragma once

// SD Card Pin
#define SD_CS_PIN

//#define STICK_C_PLUS2
#define CARDPUTER

// FLIPPER ZERO UUID's
#define FLIPPER_BLACK_UUID "00003081-0000-1000-8000-00805f9b34fb"
#define FLIPPER_WHITE_UUID "00003082-0000-1000-8000-00805f9b34fb"
#define FLIPPER_TRANSPARENT_UUID "00003083-0000-1000-8000-00805f9b34fb"

// LIGHT BLUE APP BLE SCANNER UUID's
#define BRUCE_SERVICE_UUID_0 "deadf154-0000-0000-0000-0000deadf154"     // DEADFISH

// CATHACK APPLE JUICE (uses apple UUID's need double check)
#define CATHACK_SERVICE_UUID_0 "d0611e78-bbb4-4591-a5f8-487910ae4366"   // APPLE UUID
#define CATHACK_SERVICE_UUID_1 "9fa480e0-4967-4542-9390-d343dc5d04ae"   // APPLE UUID
#define CATHACK_SERVICE_UUID_2 "7905f431-b5ce-4e99-a40f-4b1e122d00d0"   // APPLE UUID
#define CATHACK_SERVICE_UUID_3 "89d3502b-0f36-433a-8ef4-c502ad55f8dc"   // APPLE UUID
#define CATHACK_SERVICE_UUID_4 "deadf154-0000-0000-0000-0000deadf154"   // APPLE UUID
#define CATHACK_SERVICE_UUID_5 "1800"
#define CATHACK_SERVICE_UUID_6 "1801"

// Time Intervals
#define SCAN_INTERVAL_MS 5000
#define FACE_UPDATE_INTERVAL_MS 1000
#define DEVICE_SCAN_TIMEOUT 10000

// MAC Address of Known Device (example)
#define TARGET_MAC_ADDRESS "b0:81:84:96:a0:c9"

// Other Constants (e.g., for calculations)
#define RSSI_CONSTANT 20
#define DISTANCE_CONSTANT -69


const bool ENABLE_MANUFACTURER_FILTER = false;

// Ignore Manufacurer-ID's to store to SD Card
const uint16_t IGNORED_MANUFACTURERS[] = {
    0x004C, // Apple
    //0x0006, // Microsoft
    //0x0075  // Samsung
};
  
const size_t IGNORED_MANUFACTURER_COUNT = sizeof(IGNORED_MANUFACTURERS) / sizeof(IGNORED_MANUFACTURERS[0]);

inline bool isIgnoredManufacturer(uint16_t id) {
    if (!ENABLE_MANUFACTURER_FILTER) return false;
  
    for (size_t i = 0; i < IGNORED_MANUFACTURER_COUNT; ++i) {
      if (IGNORED_MANUFACTURERS[i] == id) {
        return true;
      }
    }
    return false;
}