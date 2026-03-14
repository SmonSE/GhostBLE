#include "logger.h"
#include <SD.h>

AsyncWebSocket ws("/ws");
SemaphoreHandle_t logMutex = NULL;

// Per-category target configuration
// Index = bit position of LogCategory enum
static uint8_t categoryTargets[16];

// Global category enable mask
static uint16_t enabledCategories = LOG_ALL;

// SD card log file handle (managed externally by SDLogger for structured
// writes, but the unified logger can append raw lines too)
static File logFile;
static bool sdReady = false;

void initLogger() {
    logMutex = xSemaphoreCreateMutex();

    // Default: all categories write to all targets
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

void logEnableCategory(LogCategory category) {
    enabledCategories |= (uint16_t)category;
}

void logDisableCategory(LogCategory category) {
    enabledCategories &= ~(uint16_t)category;
}

// Internal: resolve effective targets for a category
static uint8_t getTargets(LogCategory category) {
    if (!(enabledCategories & (uint16_t)category)) return TARGET_NONE;

    // Find the first matching bit and return its target config
    for (int i = 0; i < 16; i++) {
        if (category & (1 << i)) {
            return categoryTargets[i];
        }
    }
    return TARGET_ALL;
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
        Serial.println(msg);
    }
}

// Backward-compatible: existing code calls this without a category
void logToSerialAndWeb(const String& msg) {
    LOG(LOG_SYSTEM, msg);
}
