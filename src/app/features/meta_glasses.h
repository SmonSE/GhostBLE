#ifndef META_GLASSES_H
#define META_GLASSES_H

#include <NimBLEDevice.h>
#include <Arduino.h>

// ===========================================================================
//  Meta Ray-Ban Smart Glasses Detection
//  Detects Meta Ray-Ban Generation 1 + 2 Smart Glasses via BLE
//
//  Detection levels:
//    1 = Name match (moderate — could be false positive)
//    2 = Manufacturer ID 0x01AB (high)
//    3 = Service UUID 0xFD5F (definitive)
// ===========================================================================

namespace MetaGlasses {

// ── BLE identifiers ───────────────────────────────────────────
constexpr const char* META_SERVICE_UUID_FULL = "0000fd5f-0000-1000-8000-00805f9b34fb";
constexpr uint16_t    META_SERVICE_UUID_16   = 0xFD5F;
constexpr uint16_t    META_MANUFACTURER_ID   = 0x01AB;  // Meta Platforms, Inc.

// ── Statistics ────────────────────────────────────────────────
struct MetaStats {
    uint16_t glassesFound      = 0;
    uint16_t gen2Detected      = 0;
    uint16_t gen1Detected      = 0;
    uint32_t lastDetectionTime = 0;
    String   lastModelDetected;
};

extern MetaStats stats;

// ── Detection ─────────────────────────────────────────────────

// Returns true if the name matches known Meta / Ray-Ban keywords
bool hasMetaGlassesName(const String& name);

// Returns true if manufacturerId == 0x01AB
bool hasMetaManufacturerId(uint16_t manufacturerId);

// Returns true if the advertised device exposes the 0xFD5F service UUID
bool hasMetaServiceUUID(const NimBLEAdvertisedDevice* device);

// Returns true if the connected GATT client has the Meta service
bool hasMetaGATTService(NimBLEClient* pClient);

// Returns confidence level 0–3 (0 = not Meta, 3 = definitive)
uint8_t detectMetaGlasses(const NimBLEAdvertisedDevice* device,
                           const String& name,
                           uint16_t manufacturerId);

// ── Classification ────────────────────────────────────────────

// Returns "Gen 2", "Gen 1", or "Unknown"
String detectGeneration(const String& name, bool hasServiceUUID);

// Extracts model name from device name (Wayfarer, Headliner, Stories, ...)
String extractModelName(const String& name);

// Returns true if confidence >= 2 (privacy concern — glasses can record)
bool isHighValueTarget(uint8_t confidence);

// ── Reporting ─────────────────────────────────────────────────

// Logs the full detection result with confidence and model info
void logDetection(const String& devTag,
                  const String& address,
                  const String& name,
                  uint8_t       confidence,
                  const String& generation,
                  const String& model);

// Full pipeline: detect → classify → log. Call from parseDeviceInfo().
void processMetaGlasses(const NimBLEAdvertisedDevice* device,
                        const String& devTag,
                        const String& address,
                        const String& name,
                        uint16_t      manufacturerId);

// Returns formatted stats string for logging
String getStatsString();

// Resets all counters
void resetStats();

} // namespace MetaGlasses

#endif // META_GLASSES_H
