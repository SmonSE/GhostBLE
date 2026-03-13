#pragma once

#include <Arduino.h>
#include "../config/config.h"
#include "../gattServices/batteryLevelService.h"
#include "../gattServices/deviceInfoService.h"
#include "../gattServices/heartRateService.h"
#include "../gattServices/temperatureService.h"
#include "../gattServices/genericAccessService.h"
#include "../sdCard/SDLogger.h"
#include "../target/TargetDevice.h"

#include <NimBLEDevice.h>

void startBleScan();
void stopBleScan();
void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
