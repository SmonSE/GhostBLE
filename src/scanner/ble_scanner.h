#pragma once

#include <Arduino.h>
#include "config/config.h"
#include "GATTServices/reg_gatt_service.h"
#include "target/target_device.h"
#include "globals/globals.h"
#include "helper/manufacturer_helper.h"
#include "helper/service_helper.h"
#include "helper/draw_overlay.h"
#include "helper/show_expression.h"
#include "logger/logger.h"
#include "privacyCheck/device_privacy.h"
#include "core/analyzer/exposure_analyzer.h"
#include "core/models/device_info.h"
#include "privacyCheck/exposure_classifier.h"
#include "helper/ble_decoder.h"
#include "GATTServices/reg_gatt_service.h"
#include "GATTServices/pwn_beacon_service.h"
#include "core/analyzer/security_analyzer.h"
#include "gps/gps_manager.h"
#include "wardriving/wigle_logger.h"
#include "helper/nibbles_speech.h"
#include "config/scan_config.h"
#include "config/hardware.h"

#include <NimBLEDevice.h>


void startBleScan();
void stopBleScan();
void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
