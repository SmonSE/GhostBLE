#ifndef FLOCK_DETECTION_H
#define FLOCK_DETECTION_H

#include <NimBLEDevice.h>
#include <Arduino.h>

namespace FlockDetection {

// ===========================================================================
// Detection state
// ===========================================================================

enum class DetectionState {
    None,
    Suspected,
    Confirmed
};

// ===========================================================================
// Device types
// ===========================================================================

enum class FlockDeviceType {
    Unknown,
    FlockCamera,
    RavenGunshot,
    FlockExtBattery
};

// ===========================================================================
// Raven firmware families
// ===========================================================================

enum class RavenFirmware {
    Unknown,

    // Legacy Health + Location services
    V1_1_x,

    // GPS / Power / Network / Upload / Error
    V1_2_PLUS
};

// ===========================================================================
// Evidence flags
//
// These allow GhostBLE to explain WHY a device was detected.
// ===========================================================================

enum EvidenceFlags : uint16_t {

    EVIDENCE_NONE                  = 0,

    // Address / OUI
    EVIDENCE_FLOCK_OUI             = 1 << 0,
    EVIDENCE_ESPRESSIF_OUI         = 1 << 1,

    // Name
    EVIDENCE_FLOCK_NAME             = 1 << 2,
    EVIDENCE_RAVEN_NAME             = 1 << 3,

    // Manufacturer
    EVIDENCE_FLOCK_MANUFACTURER     = 1 << 4,

    // Services
    EVIDENCE_RAVEN_STANDARD_UUID    = 1 << 5,
    EVIDENCE_RAVEN_SPECIFIC_UUID    = 1 << 6,

    // Payload / future extensions
    EVIDENCE_FLOCK_PAYLOAD          = 1 << 7,

    // Multiple independent signals
    EVIDENCE_MULTI_SIGNAL           = 1 << 8
};

// ===========================================================================
// Detection result
// ===========================================================================

struct FlockResult {

    DetectionState state = DetectionState::None;

    bool detected = false;

    FlockDeviceType type = FlockDeviceType::Unknown;

    RavenFirmware ravenFW = RavenFirmware::Unknown;

    // 0 - 100
    uint8_t confidence = 0;

    // Numerical score before conversion to confidence
    uint16_t score = 0;

    // Evidence bitmask
    uint16_t evidence = EVIDENCE_NONE;

    // Matched information
    String matchedOUI;
    String matchedName;

    // Human-readable explanation
    String summary;
};

// ===========================================================================
// Statistics
// ===========================================================================

struct FlockStats {

    uint16_t flockCamerasFound = 0;
    uint16_t ravenDevicesFound = 0;
    uint16_t suspectedDevicesFound = 0;

    uint32_t lastDetectionTime = 0;

    String lastDeviceName;
};

extern FlockStats stats;

// ===========================================================================
// Detection API
// ===========================================================================

FlockResult detect(
    const NimBLEAdvertisedDevice* device,
    const String& name,
    uint16_t manufacturerId
);

// ===========================================================================
// Individual checks
// ===========================================================================

// True only for OUIs specifically associated with Flock hardware.
bool hasFlockSpecificOUI(const std::string& mac);

// True for Espressif OUIs that have been observed in Flock hardware.
// IMPORTANT: this alone does NOT identify a Flock device.
bool hasFlockEspressifOUI(const std::string& mac);

// Backwards-compatible helper.
// Returns true only for Flock-specific OUIs.
bool hasFlockOUI(const std::string& mac);

bool hasFlockName(const String& name);

bool hasRavenName(const String& name);

bool hasFlockManufacturerId(uint16_t manufacturerId);

bool hasRavenServiceUUID(
    const NimBLEAdvertisedDevice* device,
    RavenFirmware& outFW
);

// ===========================================================================
// Evidence helpers
// ===========================================================================

bool hasEvidence(
    const FlockResult& result,
    EvidenceFlags flag
);

String getEvidenceString(const FlockResult& result);

String getConfidenceString(const FlockResult& result);

String getDeviceTypeString(FlockDeviceType type);

String getDetectionStateString(DetectionState state);

// ===========================================================================
// Logging
// ===========================================================================

void logDetection(
    const String& devTag,
    const FlockResult& result,
    const String& address,
    int rssi
);

// ===========================================================================
// Stats
// ===========================================================================

String getStatsString();

void resetStats();

} // namespace FlockDetection

#endif // FLOCK_DETECTION_H
