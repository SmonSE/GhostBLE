#include "logger.h"
#include <SD.h>

AsyncWebSocket ws("/ws");
SemaphoreHandle_t logMutex = NULL;

// Per-category target configuration
// Index = bit position of LogCategory enum
static uint8_t categoryTargets[16];

// Global category enable mask
static uint16_t enabledCategories = LOG_ALL;

// Global target enable mask — controls which outputs are active
// Default: SD always on, Serial off (debug only), Web off (follows WiFi)
static uint8_t enabledTargets = TARGET_SD;

// SD card log file handle
static File logFile;
static bool sdReady = false;

void initLogger() {
    logMutex = xSemaphoreCreateMutex();

    // Default: all categories route to all targets
    // (actual output filtered by enabledTargets)
    for (int i = 0; i < 16; i++) {
        categoryTargets[i] = TARGET_ALL;
    }
}

void logSetTargets(LogCategory category, uint8_t targets) {
    for (int i = 0; i < 16; i++) {
        if (category & (1 << i)) {
            categoryTargets[i] = targets;
        }
    }
}

void logEnableTarget(uint8_t target) {
    enabledTargets |= target;
}

void logDisableTarget(uint8_t target) {
    enabledTargets &= ~target;
}

void logEnableCategory(LogCategory category) {
    enabledCategories |= (uint16_t)category;
}

void logDisableCategory(LogCategory category) {
    enabledCategories &= ~(uint16_t)category;
}

// Internal: resolve effective targets for a category
static uint8_t getTargets(LogCategory category) {
    if (!(enabledCategories & (uint16_t)category)) return TARGET_NONE;

    // Find the first matching bit and get its per-category config
    uint8_t catTargets = TARGET_ALL;
    for (int i = 0; i < 16; i++) {
        if (category & (1 << i)) {
            catTargets = categoryTargets[i];
            break;
        }
    }

    // Intersect with globally enabled targets
    return catTargets & enabledTargets;
}

// Internal: open SD log file if not already open
static void ensureSDLog() {
    if (sdReady) return;
    if (SD.exists("/GhostBLE")) {
        logFile = SD.open("/GhostBLE/log.txt", FILE_APPEND);
        sdReady = logFile ? true : false;
    }
}

void LOG(LogCategory category, const String& msg) {
    uint8_t targets = getTargets(category);
    if (targets == TARGET_NONE) return;

    if (logMutex != NULL && xSemaphoreTake(logMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        if ((targets & TARGET_SERIAL) && Serial.availableForWrite() > 0) {
            Serial.println(msg);
        }
        if ((targets & TARGET_WEB) && ws.count() > 0 && ws.availableForWriteAll()) {
            ws.textAll(msg);
        }
        if (targets & TARGET_SD) {
            ensureSDLog();
            if (sdReady && logFile) {
                logFile.println(msg);
                logFile.flush();
            }
        }
        xSemaphoreGive(logMutex);
    } else {
        // Fallback: serial only if mutex unavailable
        if (enabledTargets & TARGET_SERIAL) {
            Serial.println(msg);
        }
    }
}

// Backward-compatible wrapper
void logToSerialAndWeb(const String& msg) {
    LOG(LOG_SYSTEM, msg);
}
