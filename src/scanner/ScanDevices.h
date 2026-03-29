#pragma once

#include <Arduino.h>
#include "../config/config.h"
#include "../GATTServices/GATTServiceRegistry.h"
#include "../target/TargetDevice.h"

#include <NimBLEDevice.h>

void startBleScan();
void stopBleScan();
void scanForDevices();
void scanForDevicesTask(void* parameter);
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
