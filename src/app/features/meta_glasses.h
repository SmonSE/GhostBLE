#ifndef META_GLASSES_H
#define META_GLASSES_H

#include <NimBLEDevice.h>
#include <Arduino.h>

#include "infrastructure/logging/logger.h"


// ===========================================================================
//  Meta Ray-Ban Smart Glasses Detection
//  Detects Meta Ray-Ban Generation 2 Smart Glasses via BLE
// ===========================================================================

namespace MetaGlasses {

// Meta Ray-Ban BLE Identifiers
constexpr const char* META_SERVICE_UUID_FULL = "0000fd5f-0000-1000-8000-00805f9b34fb";
constexpr uint16_t    META_SERVICE_UUID_16   = 0xFD5F;
constexpr uint16_t    META_MANUFACTURER_ID   = 0x01AB;  // Meta/Facebook

// Known Meta Ray-Ban model identifiers in device names
static const char* META_DEVICE_NAMES[] = {
    "Meta",
    "Ray-Ban",
    "RayBan",
    "Stories",      // Gen 1 name: "Ray-Ban Stories"
    "Wayfarer",     // Model name
    "Headliner"     // Model name
};

// Statistics
struct MetaStats {
    uint16_t glassesFound      = 0;
    uint16_t gen2Detected      = 0;
    uint16_t gen1Detected      = 0;
    uint32_t lastDetectionTime = 0;
    String   lastModelDetected;
};

extern MetaStats stats;

// ===========================================================================
//  Detection Functions
// ===========================================================================

/**
 * Check if device name suggests Meta Ray-Ban glasses
 * Returns true for names containing: "Meta", "Ray-Ban", "Stories", etc.
 */
inline bool hasMetaGlassesName(const String& name) {
    if (name.isEmpty()) return false;
    
    String nameLower = name;
    nameLower.toLowerCase();
    
    for (const char* keyword : META_DEVICE_NAMES) {
        String keyLower = String(keyword);
        keyLower.toLowerCase();
        
        if (nameLower.indexOf(keyLower) != -1) {
            return true;
        }
    }
    
    return false;
}

/**
 * Check if manufacturer ID is Meta (0x01AB)
 */
inline bool hasMetaManufacturerId(uint16_t manufacturerId) {
    return (manufacturerId == META_MANUFACTURER_ID);
}

/**
 * Check if device has Meta Ray-Ban service UUID in advertisement
 */
inline bool hasMetaServiceUUID(const NimBLEAdvertisedDevice* device) {
    if (!device) return false;
    
    // Check for 16-bit service UUID
    if (device->haveServiceUUID()) {
        NimBLEUUID serviceUUID = device->getServiceUUID();
        
        // Check if it matches the Meta service UUID
        if (serviceUUID.equals(NimBLEUUID(META_SERVICE_UUID_FULL))) {
            return true;
        }
        
        // Also check 16-bit representation
        String uuidStr = serviceUUID.toString().c_str();
        if (uuidStr.indexOf("fd5f") != -1 || uuidStr.indexOf("FD5F") != -1) {
            return true;
        }
    }
    
    return false;
}

/**
 * Check if connected client has Meta Ray-Ban GATT service
 */
inline bool hasMetaGATTService(NimBLEClient* pClient) {
    if (!pClient || !pClient->isConnected()) return false;
    
    // Try to get the Meta service
    NimBLERemoteService* pService = pClient->getService(META_SERVICE_UUID_FULL);
    return (pService != nullptr);
}

/**
 * Comprehensive Meta Ray-Ban detection
 * Returns detection confidence: 0 = not Meta, 1-3 = confidence level
 */
inline uint8_t detectMetaGlasses(const NimBLEAdvertisedDevice* device, 
                                  const String& name,
                                  uint16_t manufacturerId) {
    uint8_t confidence = 0;
    
    // Level 1: Name match (weakest - could be false positive)
    if (hasMetaGlassesName(name)) {
        confidence = 1;
    }
    
    // Level 2: Manufacturer ID match (stronger)
    if (hasMetaManufacturerId(manufacturerId)) {
        confidence = 2;
    }
    
    // Level 3: Service UUID match (strongest - definitive)
    if (hasMetaServiceUUID(device)) {
        confidence = 3;
    }
    
    return confidence;
}

/**
 * Determine Meta Ray-Ban generation based on available info
 * Returns: "Gen 2", "Gen 1", or "Unknown"
 */
inline String detectGeneration(const String& name, bool hasServiceUUID) {
    // Gen 2 uses the 0xFD5F service UUID
    if (hasServiceUUID) {
        return "Gen 2";
    }
    
    // Gen 1 was called "Ray-Ban Stories"
    String nameLower = name;
    nameLower.toLowerCase();
    if (nameLower.indexOf("stories") != -1) {
        return "Gen 1";
    }
    
    // If we have Meta manufacturer ID but unclear generation
    return "Unknown";
}

/**
 * Extract model name from device name
 * E.g., "Meta Wayfarer" → "Wayfarer"
 */
inline String extractModelName(const String& name) {
    if (name.isEmpty()) return "Unknown Model";
    
    // Common patterns: "Meta Wayfarer", "Ray-Ban Headliner", etc.
    if (name.indexOf("Wayfarer") != -1) return "Wayfarer";
    if (name.indexOf("Headliner") != -1) return "Headliner";
    if (name.indexOf("Stories") != -1) return "Stories (Gen 1)";
    
    // Return full name if no specific model identified
    return name;
}

// ===========================================================================
//  Logging & Reporting
// ===========================================================================

/**
 * Log Meta Ray-Ban detection with details
 */
inline void logDetection(const String& devTag, 
                         const String& address,
                         const String& name,
                         uint8_t confidence,
                         const String& generation,
                         const String& model) {
    String confidenceStr;
    switch (confidence) {
        case 3: confidenceStr = "DEFINITIVE (Service UUID)"; break;
        case 2: confidenceStr = "HIGH (Manufacturer ID)"; break;
        case 1: confidenceStr = "MODERATE (Name match)"; break;
        default: confidenceStr = "NONE"; break;
    }
    
    String logMsg = devTag + "👓 META RAY-BAN DETECTED!\n"
        "   Address:     " + address + "\n"
        "   Name:        " + name + "\n"
        "   Model:       " + model + "\n"
        "   Generation:  " + generation + "\n"
        "   Confidence:  " + confidenceStr;
    
    LOG(LOG_TARGET, logMsg);
    
    // Update statistics
    stats.glassesFound++;
    stats.lastDetectionTime = millis();
    stats.lastModelDetected = model;
    
    if (generation == "Gen 2") {
        stats.gen2Detected++;
    } else if (generation == "Gen 1") {
        stats.gen1Detected++;
    }
}

/**
 * Process Meta Ray-Ban detection and logging
 * Call this after detecting Meta glasses
 */
inline void processMetaGlasses(const NimBLEAdvertisedDevice* device,
                               const String& devTag,
                               const String& address,
                               const String& name,
                               uint16_t manufacturerId) {
    // Detect with confidence level
    uint8_t confidence = detectMetaGlasses(device, name, manufacturerId);
    
    if (confidence == 0) return;  // Not Meta glasses
    
    // Determine generation
    bool hasServiceUUID = hasMetaServiceUUID(device);
    String generation = detectGeneration(name, hasServiceUUID);
    
    // Extract model
    String model = extractModelName(name);
    
    // Log the detection
    logDetection(devTag, address, name, confidence, generation, model);
}

/**
 * Get statistics as formatted string
 */
inline String getStatsString() {
    String s = "👓 META RAY-BAN STATS:\n";
    s += "   Total found:    " + String(stats.glassesFound) + "\n";
    s += "   Gen 2:          " + String(stats.gen2Detected) + "\n";
    s += "   Gen 1:          " + String(stats.gen1Detected) + "\n";
    
    if (!stats.lastModelDetected.isEmpty()) {
        s += "   Last model:     " + stats.lastModelDetected;
    }
    
    if (stats.lastDetectionTime > 0) {
        s += "\n   Last detected:  " + String((millis() - stats.lastDetectionTime) / 1000) + "s ago";
    }
    
    return s;
}

/**
 * Reset statistics
 */
inline void resetStats() {
    stats.glassesFound      = 0;
    stats.gen2Detected      = 0;
    stats.gen1Detected      = 0;
    stats.lastDetectionTime = 0;
    stats.lastModelDetected = "";
}

/**
 * Check if this is a high-value target (privacy concern)
 * Meta glasses can record video/audio without clear indication
 */
inline bool isHighValueTarget(uint8_t confidence) {
    // Only flag as high-value if we're confident it's actually Meta glasses
    return (confidence >= 2);  // Manufacturer ID or Service UUID match
}

} // namespace MetaGlasses

#endif // META_GLASSES_H
