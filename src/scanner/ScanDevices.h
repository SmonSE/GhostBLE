#pragma once

#include <Arduino.h>
#include "../config/config.h"
#include "../GATTServices/batteryLevelService.h"
#include "../GATTServices/deviceInfoService.h"
#include "../GATTServices/heartRateService.h"
#include "../GATTServices/temperatureService.h"
#include "../GATTServices/genericAccessService.h"
#include "../GATTServices/currentTimeService.h"
#include "../target/TargetDevice.h"

#include <NimBLEDevice.h>

void startBleScan();
void stopBleScan();
void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
