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

static bool sdInitialized = false;

// Category index → filename mapping
static const char* catFileNames[] = {
    "/GhostBLE/scan.log",       // 0  LOG_SCAN
    "/GhostBLE/gatt.log",       // 1  LOG_GATT
    "/GhostBLE/privacy.log",    // 2  LOG_PRIVACY
    "/GhostBLE/security.log",   // 3  LOG_SECURITY
    "/GhostBLE/beacon.log",     // 4  LOG_BEACON
    "/GhostBLE/control.log",    // 5  LOG_CONTROL
    "/GhostBLE/gps.log",        // 6  LOG_GPS
    "/GhostBLE/system.log",     // 7  LOG_SYSTEM
    "/GhostBLE/target.log",     // 8  LOG_TARGET
    "/GhostBLE/notify.log",     // 9  LOG_NOTIFY
    "/GhostBLE/misc.log",       // 10 (unused)
    "/GhostBLE/misc.log",       // 11
    "/GhostBLE/misc.log",       // 12
    "/GhostBLE/misc.log",       // 13
    "/GhostBLE/misc.log",       // 14
    "/GhostBLE/misc.log",       // 15
};

static void migrateToFolder() {
    SD.mkdir("/GhostBLE");
    Serial.println("#Logger# Created /GhostBLE folder, migrating legacy files...");

    if (SD.exists("/device_info.txt")) {
        SD.rename("/device_info.txt", "/GhostBLE/device_info.txt");
        Serial.println("#Logger# Migrated /device_info.txt");
    }

    for (int i = 1; i <= 9999; i++) {
        char oldPath[24];
        snprintf(oldPath, sizeof(oldPath), "/wigle_%04d.csv", i);
        if (!SD.exists(oldPath)) break;
        char newPath[34];
        snprintf(newPath, sizeof(newPath), "/GhostBLE/wigle_%04d.csv", i);
        SD.rename(oldPath, newPath);
    }

    if (SD.exists("/wigle_overflow.csv")) {
        SD.rename("/wigle_overflow.csv", "/GhostBLE/wigle_overflow.csv");
    }

    // Remove legacy single log file
    if (SD.exists("/GhostBLE/log.txt")) {
        SD.remove("/GhostBLE/log.txt");
    }
}

bool initLogger(int sdCsPin) {
    logMutex = xSemaphoreCreateMutex();

    // Disable not important LOGs to reduce trace load
    logDisableCategory(LOG_SYSTEM);
    logDisableCategory(LOG_CONTROL);

    // Default: all categories route to all targets
    // (actual output filtered by enabledTargets)
    for (int i = 0; i < 16; i++) {
        categoryTargets[i] = TARGET_ALL;
    }

    // Initialize SD card (skip on devices without SD slot)
    if (sdCsPin >= 0) {
        bool sdOk = SD.begin(sdCsPin);
        if (!sdOk) {
            Serial.println("#Logger# SD card initialization failed!");
            return false;
        }

        // Create folder and migrate legacy files on first run
        if (!SD.exists("/GhostBLE")) {
            migrateToFolder();
        }

        sdInitialized = true;
        Serial.println("#Logger# SD card ready.");
    } else {
        // No SD card — disable SD logging target
        enabledTargets &= ~TARGET_SD;
        Serial.println("#Logger# No SD card slot on this device, SD logging disabled.");
    }
    return true;
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

void logNewBoot() {
    char msg[64];

    for (int i = 0; i < 16; i++) {
        // Skip unused categories (mapped to misc)
        if (strcmp(catFileNames[i], "/GhostBLE/misc.log") == 0) continue;

        LogCategory cat = (LogCategory)(1 << i);

        if (!(enabledCategories & (uint16_t)cat)) continue;

        LOG(cat, "");
        LOG(cat, "==================================================");
        snprintf(msg, sizeof(msg), "==== [BOOT] NEW BOOT [%s] ====", catFileNames[i]);
        LOG(cat, msg);
        LOG(cat, "==================================================");
        LOG(cat, "");
    }
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

// Internal: get category filename
static const char* getCatFileName(LogCategory category) {
    for (int i = 0; i < 16; i++) {
        if (category & (1 << i)) return catFileNames[i];
    }
    return catFileNames[7]; // fallback to system.log
}

void LOG(LogCategory category, const String& msg) {
    uint8_t targets = getTargets(category);
    if (targets == TARGET_NONE) return;

    if (logMutex != NULL && xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if ((targets & TARGET_SERIAL) && Serial.availableForWrite() > 0) {
            Serial.println(msg);
        }
        if ((targets & TARGET_WEB) && ws.count() > 0 && ws.availableForWriteAll()) {
            ws.textAll(msg);
        }
        if ((targets & TARGET_SD) && sdInitialized) {
            File f = SD.open(getCatFileName(category), FILE_APPEND);
            if (f) {
                f.println(msg);
                f.close();
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
