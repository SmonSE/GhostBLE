#include "scanConfig.h"
#include "../helper/showScanIcon.h"
#include <NimBLEDevice.h>

// ===== Global State =====
ScanMode currentMode = AGGRESSIVE;

// ===== RSSI Thresholds =====
int RSSI_IGNORE_THRESHOLD = -99;
int RSSI_CONNECT_THRESHOLD = -94;

// ===== BLE Scan Parameters =====
int BLE_SCAN_INTERVAL = 45;
int BLE_SCAN_WINDOW   = 30;

// ===== Apply Mode =====
void applyScanMode() {

  switch (currentMode) {

    case FOCUSED:
      RSSI_IGNORE_THRESHOLD  = -90;
      RSSI_CONNECT_THRESHOLD = -85;

      BLE_SCAN_INTERVAL = 45;
      BLE_SCAN_WINDOW   = 20;
      break;

    case BALANCED:
      RSSI_IGNORE_THRESHOLD  = -95;
      RSSI_CONNECT_THRESHOLD = -90;

      BLE_SCAN_INTERVAL = 45;
      BLE_SCAN_WINDOW   = 25;
      break;

    case AGGRESSIVE:
      RSSI_IGNORE_THRESHOLD  = -99;
      RSSI_CONNECT_THRESHOLD = -94;

      BLE_SCAN_INTERVAL = 45;
      BLE_SCAN_WINDOW   = 30;
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