#ifndef FLOCK_DETECTION_H
#define FLOCK_DETECTION_H

#include <NimBLEDevice.h>
#include <Arduino.h>

// ===========================================================================
//  Flock Safety Camera & Raven Gunshot Detector Detection
//
//  Detection methods (BLE-only, passive — no connection required):
//    1. MAC OUI prefix  — 20+ known Flock Safety OUI prefixes
//    2. Device name     — "FS Ext Battery", "Penguin", "Flock", "Pigvision"
//    3. Manufacturer ID — 0x09C8 (XUNTONG) used in Flock hardware
//    4. Raven UUID      — SoundThinking/ShotSpotter service UUIDs
//    5. Raven FW est.   — firmware version from UUID pattern
//
//  Sources:
//    colonelpanichacks/flock-you (MIT)
//    wgreenberg/flock-you  — manufacturer ID 0x09C8
//    GainSec — Raven BLE service UUID dataset
//    DeFlock / deflock.me — crowdsourced OUI list
// ===========================================================================

namespace FlockDetection {

// ── Device types ──────────────────────────────────────────────
enum class FlockDeviceType {
    Unknown,
    FlockCamera,        // Flock Safety ALPR camera
    RavenGunshot,       // SoundThinking/ShotSpotter Raven
    FlockExtBattery,    // External battery unit
};

// ── Raven firmware versions ───────────────────────────────────
enum class RavenFirmware {
    Unknown,
    V1_1_x,   // has legacy Health + Location service
    V1_2_x,   // has GPS + Power + Network + Upload + Error
    V1_3_x,   // same as 1.2.x + extended services
};

// ── Detection result ──────────────────────────────────────────
struct FlockResult {
    bool            detected   = false;
    FlockDeviceType type       = FlockDeviceType::Unknown;
    RavenFirmware   ravenFW    = RavenFirmware::Unknown;

    uint8_t         confidence = 0;   // 1=name/OUI, 2=mfr ID, 3=UUID

    String          matchedOUI;       // matched MAC prefix if any
    String          matchedName;      // matched name keyword if any
    String          summary;          // human-readable detection string
};

// ── Statistics ────────────────────────────────────────────────
struct FlockStats {
    uint16_t flockCamerasFound = 0;
    uint16_t ravenDevicesFound = 0;
    uint32_t lastDetectionTime = 0;
    String   lastDeviceName;
};

extern FlockStats stats;

// ── Detection API (passive — no BLE connection needed) ────────

// Full detection pipeline — call from parseDeviceInfo().
// Returns a FlockResult with detected=false if not a Flock/Raven device.
FlockResult detect(const NimBLEAdvertisedDevice* device,
                   const String&   name,
                   uint16_t        manufacturerId);

// Individual checks (used internally, exposed for testing)
bool hasFlockOUI(const std::string& mac);
bool hasFlockName(const String& name);
bool hasFlockManufacturerId(uint16_t manufacturerId);
bool hasRavenServiceUUID(const NimBLEAdvertisedDevice* device,
                         RavenFirmware& outFW);

// ── Logging ───────────────────────────────────────────────────
void logDetection(const String& devTag, const FlockResult& result,
                  const String& address, int rssi);

// ── Stats ─────────────────────────────────────────────────────
String getStatsString();
void   resetStats();

} // namespace FlockDetection

#endif // FLOCK_DETECTION_H
