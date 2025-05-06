#ifndef SCANDEVICES_H
#define SCANDEVICES_H

#include <Arduino.h>
#include <ArduinoBLE.h>
#include "../config/config.h"
#include "../bleServices/batteryLevelService.h"
#include "../bleServices/heartRateService.h"
#include "../bleServices/currentTimeService.h"
#include "../bleServices/deviceInfoService.h"
#include "../sdCard/SDLogger.h"
#include "../target/TargetDevice.h"

void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showLastConnectedDevice();
void showFindingCounter(int sniffed, int spotted);

#endif // SCANDEVICES_H
