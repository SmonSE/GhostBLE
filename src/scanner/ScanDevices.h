#ifndef SCANDEVICES_H
#define SCANDEVICES_H

#include <Arduino.h>
#include "../config/config.h"
#include "../bleServices/batteryLevelService.h"
#include "../bleServices/heartRateService.h"
#include "../bleServices/deviceInfoService.h"
#include "../sdCard/SDLogger.h"
#include "../target/TargetDevice.h"

#include <NimBLEDevice.h>

extern NimBLEScan* pBLEScan;

void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);

#endif // SCANDEVICES_H
