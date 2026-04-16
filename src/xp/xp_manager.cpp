#include "xp_manager.h"
#include <math.h>
#include "logger/logger.h"

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
    return (uint32_t)floorf(16.67f * powf((float)level, 1.477f));
}

void XPManager::recalculateLevel() {
    while (totalXP >= xpForLevel(currentLevel + 1)) {
        currentLevel++;
    }
}

const char* XPManager::getTitle() {
    uint16_t lvl = currentLevel;

    // --- Mythic BLE Tier ---
    if (lvl >= 300) return "Null Advertiser";
    if (lvl >= 270) return "Hidden Broadcaster";
    if (lvl >= 250) return "RF Ascendant";
    if (lvl >= 230) return "Packet God";
    if (lvl >= 210) return "BLE Revenant";
    if (lvl >= 195) return "Airwave Ghost";
    if (lvl >= 180) return "Silent Beacon";
    if (lvl >= 165) return "Phantom Peripheral";
    if (lvl >= 150) return "Spectrum Deity";
    if (lvl >= 135) return "Ether Controller";

    // --- Power Tier ---
    if (lvl >= 120) return "GATT Architect";
    if (lvl >= 110) return "Beacon Overlord";
    if (lvl >= 100) return "RF Dominator";
    if (lvl >= 92)  return "Advert Injector";
    if (lvl >= 85)  return "Frequency Warlord";
    if (lvl >= 78)  return "Signal Overmind";
    if (lvl >= 70)  return "BLE Phantom";
    if (lvl >= 63)  return "Packet Architect";
    if (lvl >= 56)  return "Airwave Manipulator";

    // --- Hacker Tier ---
    if (lvl >= 50)  return "Spectrum Rider";
    if (lvl >= 45)  return "BLE Operator";
    if (lvl >= 40)  return "Data Phreak";
    if (lvl >= 36)  return "RF Trickster";
    if (lvl >= 32)  return "Frame Breaker";
    if (lvl >= 28)  return "UUID Tracker";
    if (lvl >= 24)  return "Signal Tinkerer";
    if (lvl >= 20)  return "Packet Whisperer";
    if (lvl >= 16)  return "Protocol Raider";

    // --- Learning Tier ---
    if (lvl >= 14)  return "Beacon Chaser";
    if (lvl >= 12)  return "Ping Listener";
    if (lvl >= 10)  return "Airwave Scout";
    if (lvl >= 8)   return "RF Wanderer";
    if (lvl >= 6)   return "Byte Rookie";
    if (lvl >= 4)   return "Signal Sniffer";
    if (lvl >= 2)   return "Packet Peeker";
    if (lvl >= 1)   return "Curious Scanner";

    return "Uninitialized Device";
}
