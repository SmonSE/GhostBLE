#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;
extern SemaphoreHandle_t logMutex;

// ===== Log Categories (bitmask) =====
enum LogCategory : uint16_t {
    LOG_NONE       = 0,
    LOG_SCAN       = (1 << 0),   // Device discovery, scan start/stop
    LOG_GATT       = (1 << 1),   // GATT service reads, characteristic data
    LOG_PRIVACY    = (1 << 2),   // Privacy analysis, exposure scoring
    LOG_SECURITY   = (1 << 3),   // Security findings, vulnerabilities
    LOG_BEACON     = (1 << 4),   // iBeacon data
    LOG_CONTROL    = (1 << 5),   // WiFi/BLE toggle, wardriving, user actions
    LOG_GPS        = (1 << 6),   // GPS data, satellite info
    LOG_SYSTEM     = (1 << 7),   // Boot, init, errors, memory
    LOG_TARGET     = (1 << 8),   // Suspicious/target device detection
    LOG_NOTIFY     = (1 << 9),   // BLE notification data
    LOG_ALL        = 0xFFFF
};

// ===== Log Targets (bitmask) =====
enum LogTarget : uint8_t {
    TARGET_NONE    = 0,
    TARGET_SERIAL  = (1 << 0),
    TARGET_WEB     = (1 << 1),
    TARGET_SD      = (1 << 2),
    TARGET_ALL     = 0xFF
};

// ===== Configuration =====
void initLogger();

// Set which categories are enabled for each target
void logSetTargets(LogCategory category, uint8_t targets);

// Enable/disable individual targets globally
void logEnableTarget(uint8_t target);
void logDisableTarget(uint8_t target);

// Enable/disable a category globally
void logEnableCategory(LogCategory category);
void logDisableCategory(LogCategory category);

// ===== Logging Functions =====

// Unified log: writes to configured targets based on category
void LOG(LogCategory category, const String& msg);

