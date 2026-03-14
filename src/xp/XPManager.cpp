#include "XPManager.h"
#include <math.h>
#include "../logger/logger.h"

static const char* XP_FILE = "/GhostBLE/xp.dat";

void XPManager::begin() {
    File f = SD.open(XP_FILE, FILE_READ);
    if (f) {
        XPData data;
        if (f.read((uint8_t*)&data, sizeof(data)) == sizeof(data) && data.magic == XP_MAGIC) {
            totalXP = data.totalXP;
            currentLevel = data.currentLevel;
            LOG(LOG_SYSTEM, "XP loaded: " + String(totalXP) + " XP, LVL " + String(currentLevel));
        } else {
            LOG(LOG_SYSTEM, "XP file invalid, starting fresh");
        }
        f.close();
    } else {
        LOG(LOG_SYSTEM, "No XP file found, starting at LVL 1");
    }
    recalculateLevel();
    lastSaveTime = millis();
}

void XPManager::awardXP(uint16_t amount) {
    totalXP += amount;
    dirty = true;

    uint16_t oldLevel = currentLevel;
    recalculateLevel();

    if (currentLevel > oldLevel) {
        LOG(LOG_SYSTEM, "LEVEL UP! LVL " + String(oldLevel) + " -> " + String(currentLevel) + " (XP: " + String(totalXP) + ")");
        save();  // Force save on level-up
    }
}

void XPManager::save() {
    if (!dirty) return;

    unsigned long now = millis();
    if (now - lastSaveTime < XP_SAVE_INTERVAL) return;

    XPData data;
    data.magic = XP_MAGIC;
    data.totalXP = totalXP;
    data.currentLevel = currentLevel;

    File f = SD.open(XP_FILE, FILE_WRITE);
    if (f) {
        f.write((uint8_t*)&data, sizeof(data));
        f.close();
        dirty = false;
        lastSaveTime = now;
        LOG(LOG_SYSTEM, "XP saved: " + String(totalXP) + " XP, LVL " + String(currentLevel));
    }
}

uint16_t XPManager::getLevel() {
    return currentLevel;
}

uint32_t XPManager::getXP() {
    return totalXP;
}

uint8_t XPManager::getProgressPercent() {
    uint32_t currentLevelXP = xpForLevel(currentLevel);
    uint32_t nextLevelXP = xpForLevel(currentLevel + 1);
    uint32_t range = nextLevelXP - currentLevelXP;
    if (range == 0) return 100;
    uint32_t progress = totalXP - currentLevelXP;
    return (uint8_t)((progress * 100) / range);
}

uint32_t XPManager::xpForLevel(uint16_t level) {
    if (level <= 1) return 0;
    return (uint32_t)floor(16.67 * pow((double)level, 1.477));
}

void XPManager::recalculateLevel() {
    while (totalXP >= xpForLevel(currentLevel + 1)) {
        currentLevel++;
    }
}

const char* XPManager::getTitle() {
    uint16_t lvl = currentLevel;
    if (lvl >= 500) return "BLE Deity";
    if (lvl >= 300) return "Phantom Lord";
    if (lvl >= 200) return "Ghost King";
    if (lvl >= 150) return "Ether Wraith";
    if (lvl >= 100) return "Packet Overlord";
    if (lvl >= 75)  return "Signal Sorcerer";
    if (lvl >= 50)  return "Byte Bandit";
    if (lvl >= 35)  return "Freq Fiend";
    if (lvl >= 25)  return "Sniff Lord";
    if (lvl >= 18)  return "Air Pirate";
    if (lvl >= 12)  return "Beacon Hunter";
    if (lvl >= 8)   return "Data Gremlin";
    if (lvl >= 5)   return "Sniff Puppy";
    if (lvl >= 3)   return "Bit Curious";
    return "Clueless Ghost";
}
