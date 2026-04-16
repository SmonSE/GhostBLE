#include "scanConfig.h"
#include "helper/showScanIcon.h"
#include <NimBLEDevice.h>

// ===== Global State =====
ScanMode currentMode = AGGRESSIVE;

// ===== RSSI Thresholds =====
int RSSI_IGNORE_THRESHOLD = -100;
int RSSI_CONNECT_THRESHOLD = -98;

// ===== BLE Scan Parameters =====
int BLE_SCAN_INTERVAL = 45;
int BLE_SCAN_WINDOW   = 30;

// ===== Apply Mode =====
void applyScanMode() {

  switch (currentMode) {

    case FOCUSED:
      RSSI_IGNORE_THRESHOLD  = -85;
      RSSI_CONNECT_THRESHOLD = -80;

      BLE_SCAN_INTERVAL = 90;
      BLE_SCAN_WINDOW   = 30;
      break;

    case BALANCED:
      RSSI_IGNORE_THRESHOLD  = -95;
      RSSI_CONNECT_THRESHOLD = -92;

      BLE_SCAN_INTERVAL = 60;
      BLE_SCAN_WINDOW   = 45;
      break;

    case AGGRESSIVE:
      RSSI_IGNORE_THRESHOLD  = -100;
      RSSI_CONNECT_THRESHOLD = -98;

      BLE_SCAN_INTERVAL = 30 + random(0, 10);
      BLE_SCAN_WINDOW   = BLE_SCAN_INTERVAL;
      break;
  }

  showScanIcon();
}

// ===== Toggle Mode =====
void toggleScanMode() {
  currentMode = (ScanMode)((currentMode + 1) % 3);
  // Serial print mode:
  Serial.println("CurrentMode: " + currentMode);
  applyScanMode();
}

// ===== Helper für UI =====
const char* getScanModeName() {
  switch (currentMode) {
    case FOCUSED:    return "FOCUS";
    case BALANCED:   return "BAL";
    case AGGRESSIVE: return "AGGR";
    default:         return "?";
  }
}