#pragma once

#include <Arduino.h>
#include "../config/config.h"
#include "../GATTServices/GATTServiceRegistry.h"
#include "../target/TargetDevice.h"

#include <NimBLEDevice.h>

// Scan state machine (non-blocking)
enum ScanState : uint8_t {
  SCAN_IDLE,           // Waiting for next scan cycle
  SCAN_ACTIVE,         // NimBLE scan running in background
  SCAN_ADV_WAIT,       // Re-advertising PwnBeacon before processing
  SCAN_PROCESSING,     // Processing discovered devices one at a time
};

extern ScanState scanState;

void initBleScan();
void startBleScan();
void stopBleScan();
void updateBleScan();   // Call from loop() — drives the state machine
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
