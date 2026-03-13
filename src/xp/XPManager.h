#pragma once

#include <Arduino.h>
#include <SD.h>

#define XP_MAGIC 0x58504C56
#define XP_SAVE_INTERVAL 60000  // 60 seconds

class XPManager {
public:
    void begin();
    void awardXP(uint16_t amount);
    void save();

    uint16_t getLevel();
    uint32_t getXP();
    uint8_t getProgressPercent();
    const char* getTitle();

private:
    struct __attribute__((packed)) XPData {
        uint32_t magic;
        uint32_t totalXP;
        uint16_t currentLevel;
    };

    uint32_t totalXP = 0;
    uint16_t currentLevel = 1;
    unsigned long lastSaveTime = 0;
    bool dirty = false;

    uint32_t xpForLevel(uint16_t level);
    void recalculateLevel();
};
