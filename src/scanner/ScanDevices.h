#pragma once

#include <Arduino.h>
#include "config/config.h"
#include "GATTServices/GATTServiceRegistry.h"
#include "target/TargetDevice.h"
#include "globals/globals.h"
#include "helper/ManufacturerHelper.h"
#include "helper/ServiceHelper.h"
#include "helper/drawOverlay.h"
#include "helper/showExpression.h"
#include "logger/logger.h"
#include "privacyCheck/devicePrivacy.h"
#include "core/analyzer/ExposureAnalyzer.h"
#include "core/models/DeviceInfo.h"
#include "privacyCheck/ExposureClassifier.h"
#include "helper/BLEDecoder.h"
#include "GATTServices/GATTServiceRegistry.h"
#include "GATTServices/pwnBeaconService.h"
#include "core/analyzer/SecurityAnalyzer.h"
#include "gps/GPSManager.h"
#include "wardriving/WigleLogger.h"
#include "helper/nibblesSpeech.h"
#include "config/scanConfig.h"
#include "config/hardware.h"

#include <NimBLEDevice.h>


void startBleScan();
void stopBleScan();
void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
