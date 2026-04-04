#pragma once

#include <Arduino.h>

// ===== Scan Modes =====
enum ScanMode {
  FOCUSED = 0,
  BALANCED,
  AGGRESSIVE
};

// ===== Global State =====
extern ScanMode currentMode;

// ===== RSSI Thresholds =====
extern int RSSI_IGNORE_THRESHOLD;
extern int RSSI_CONNECT_THRESHOLD;

// ===== BLE Scan Parameters =====
extern int BLE_SCAN_INTERVAL;
extern int BLE_SCAN_WINDOW;

// ===== Mode Control =====
void applyScanMode();
void toggleScanMode();

// ===== Optional Helpers =====
const char* getScanModeName();