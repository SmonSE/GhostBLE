#pragma once

#include <Arduino.h>
#include "config/signal_config.h"
#include "config/gatt_config.h"
#include "config/app_config.h"
#include "gattServices/reg_gatt_service.h"
#include "core/detection/target_device.h"
#include "app/context/globals.h"
#include "core/parsing/manufacturer_parser.h"
#include "core/parsing/service_parser.h"
#include "ui/overlay/draw_overlay.h"
#include "ui/expression/show_expression.h"
#include "infrastructure/ble/ble_scanner.h"
#include "core/privacy/device_privacy.h"
#include "core/analyzer/exposure_analyzer.h"
#include "core/models/device_info.h"
#include "core/privacy/exposure_classifier.h"
#include "core/parsing/ble_decoder.h"
#include "gattServices/reg_gatt_service.h"
#include "gattServices/pwn_beacon_service.h"
#include "core/analyzer/security_analyzer.h"
#include "infrastructure/gps/gps_manager.h"
#include "infrastructure/wardriving/wigle_logger.h"
#include "app/interaction/nibbles_speech.h"
#include "config/scan_config.h"
#include "infrastructure/platform/hardware.h"

#include <NimBLEDevice.h>
#include "core/analyzer/device_registry.h"
#include "core/analyzer/exposure_analyzer.h"
#include "core/analyzer/soft_fingerprint.h"

#include "utils/apple_models.h"


class BleScanner {
private:
    DeviceRegistry registry;
};

void startBleScan();
void stopBleScan();
void scanForDevices();
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
