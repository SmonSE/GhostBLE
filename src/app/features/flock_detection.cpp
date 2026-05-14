#include "flock_detection.h"
#include "infrastructure/logging/logger.h"

namespace FlockDetection {

// ── Stats instance ─────────────────────────────────────────────
FlockStats stats;

// ============================================================
//  Flock Safety MAC OUI prefixes
//  Source: DeFlock / deflock.me crowdsourced dataset +
//          colonelpanichacks/flock-you (20 verified prefixes)
//
//  Format: first 3 bytes of MAC as lowercase "xx:xx:xx"
// ============================================================
static const char* FLOCK_OUIS[] = {
    // Flock Safety primary OUIs
    "00:e0:4c",   // Realtek — used in early Flock hardware
    "10:02:b5",   // Flock Safety
    "18:fe:34",   // Espressif (ESP32-based units)
    "24:0a:c4",   // Espressif
    "24:6f:28",   // Espressif
    "2c:f4:32",   // Espressif
    "30:ae:a4",   // Espressif
    "3c:61:05",   // Espressif
    "40:22:d8",   // Espressif
    "40:f5:20",   // Espressif
    "48:3f:da",   // Flock WiFi module
    "4c:11:ae",   // Flock Safety ext battery
    "50:02:91",   // Flock Safety
    "54:43:b2",   // Espressif
    "58:bf:25",   // Espressif
    "5c:cf:7f",   // Espressif (legacy)
    "60:55:f9",   // Flock Safety
    "68:b6:b3",   // Flock Safety
    "7c:9e:bd",   // Flock Safety
    "82:6b:f2",   // DeFlockJoplin 31st OUI
    "84:0d:8e",   // Espressif
    "84:cc:a8",   // Espressif
    "8c:aa:b5",   // Espressif
    "90:38:0c",   // Flock Safety
    "a4:cf:12",   // Espressif
    "a4:e5:7c",   // Flock Safety
    "b4:e6:2d",   // Espressif
    "c8:f0:9e",   // Flock Safety camera
    "cc:50:e3",   // Espressif
    "d8:a0:1d",   // Espressif
    "dc:54:75",   // Espressif
    "e8:68:e7",   // Flock Safety
    "ec:62:60",   // Espressif
    "f0:08:d1",   // Flock Safety
};
static constexpr size_t FLOCK_OUI_COUNT =
    sizeof(FLOCK_OUIS) / sizeof(FLOCK_OUIS[0]);

// ============================================================
//  Flock Safety BLE device name keywords
//  Case-insensitive substring match
// ============================================================
static const char* FLOCK_NAMES[] = {
    "flock",
    "fs ext battery",
    "fs-ext",
    "penguin",       // Flock camera internal codename
    "pigvision",     // Flock/partner branding
    "raven",         // SoundThinking/ShotSpotter Raven
    "shotspotter",
    "soundthinking",
};
static constexpr size_t FLOCK_NAME_COUNT =
    sizeof(FLOCK_NAMES) / sizeof(FLOCK_NAMES[0]);

// ============================================================
//  Flock Safety manufacturer ID
//  0x09C8 = XUNTONG — used in Flock hardware modules.
//  Source: wgreenberg/flock-you
// ============================================================
static constexpr uint16_t FLOCK_MANUFACTURER_ID = 0x09C8;

// ============================================================
//  Raven (SoundThinking/ShotSpotter) BLE service UUIDs
//  Source: GainSec — raven_configurations.json dataset
//
//  Firmware 1.1.x uses legacy Health + Location services.
//  Firmware 1.2.x adds GPS, Power, Network, Upload, Error.
//  Firmware 1.3.x same as 1.2.x + extended services.
//
//  Only the short prefix is matched — full 128-bit UUID is
//  0000XXXX-0000-1000-8000-00805f9b34fb standard form.
// ============================================================

// Raven 1.1.x legacy services
static const char* RAVEN_UUIDS_V1_1[] = {
    "00001809",   // Health Thermometer (legacy health service)
    "00001819",   // Location and Navigation (legacy location service)
};
static constexpr size_t RAVEN_V1_1_COUNT =
    sizeof(RAVEN_UUIDS_V1_1) / sizeof(RAVEN_UUIDS_V1_1[0]);

// Raven 1.2.x+ services
static const char* RAVEN_UUIDS_V1_2[] = {
    "00003100",   // GPS service — real-time coordinates
    "00003200",   // Power service — battery & solar status
    "00003300",   // Network service — LTE/WiFi connectivity
    "00003400",   // Upload service — data transmission metrics
    "00003500",   // Error service — diagnostics & error logs
};
static constexpr size_t RAVEN_V1_2_COUNT =
    sizeof(RAVEN_UUIDS_V1_2) / sizeof(RAVEN_UUIDS_V1_2[0]);

// ============================================================
//  Helper: check if a UUID string contains a known prefix
// ============================================================
static bool uuidStartsWith(const std::string& uuid, const char* prefix) {
    // NimBLE UUID toString() returns lowercase "0000xxxx-..."
    // or "0x3100" for short-form UUIDs.
    return uuid.find(prefix) != std::string::npos;
}

// ============================================================
//  Individual detection methods
// ============================================================

bool hasFlockOUI(const std::string& mac) {
    if (mac.length() < 8) return false;

    // Extract first 8 chars: "xx:xx:xx"
    std::string prefix = mac.substr(0, 8);

    // Lowercase for comparison
    for (char& c : prefix) c = tolower(c);

    for (size_t i = 0; i < FLOCK_OUI_COUNT; i++) {
        if (prefix == FLOCK_OUIS[i]) return true;
    }
    return false;
}

bool hasFlockName(const String& name) {
    if (name.isEmpty()) return false;

    String lower = name;
    lower.toLowerCase();

    for (size_t i = 0; i < FLOCK_NAME_COUNT; i++) {
        if (lower.indexOf(FLOCK_NAMES[i]) != -1) return true;
    }
    return false;
}

bool hasFlockManufacturerId(uint16_t manufacturerId) {
    return manufacturerId == FLOCK_MANUFACTURER_ID;
}

bool hasRavenServiceUUID(const NimBLEAdvertisedDevice* device,
                         RavenFirmware& outFW) {
    if (!device) return false;

    int  svcCount  = device->getServiceUUIDCount();
    int  v11Hits   = 0;
    int  v12Hits   = 0;

    for (int i = 0; i < svcCount; i++) {
        std::string uuid = device->getServiceUUID(i).toString();

        // Check 1.1.x legacy UUIDs
        for (size_t j = 0; j < RAVEN_V1_1_COUNT; j++) {
            if (uuidStartsWith(uuid, RAVEN_UUIDS_V1_1[j])) v11Hits++;
        }

        // Check 1.2.x+ UUIDs
        for (size_t j = 0; j < RAVEN_V1_2_COUNT; j++) {
            if (uuidStartsWith(uuid, RAVEN_UUIDS_V1_2[j])) v12Hits++;
        }
    }

    // Need at least 2 matching UUIDs to avoid false positives
    if (v12Hits >= 2) {
        outFW = RavenFirmware::V1_2_x;
        return true;
    }
    if (v11Hits >= 2) {
        outFW = RavenFirmware::V1_1_x;
        return true;
    }

    return false;
}

// ============================================================
//  Main detection pipeline
// ============================================================
FlockResult detect(const NimBLEAdvertisedDevice* device,
                   const String&   name,
                   uint16_t        manufacturerId) {
    FlockResult result;

    if (!device) return result;

    std::string mac = device->getAddress().toString();

    // ── Level 3: Raven service UUID (definitive) ─────────────
    RavenFirmware ravenFW = RavenFirmware::Unknown;
    if (hasRavenServiceUUID(device, ravenFW)) {
        result.detected    = true;
        result.type        = FlockDeviceType::RavenGunshot;
        result.ravenFW     = ravenFW;
        result.confidence  = 3;
        result.summary     = "SoundThinking/ShotSpotter Raven (gunshot detector)";

        switch (ravenFW) {
            case RavenFirmware::V1_1_x:
                result.summary += " — FW ~1.1.x (legacy services)";
                break;
            case RavenFirmware::V1_2_x:
                result.summary += " — FW ~1.2.x/1.3.x";
                break;
            default: break;
        }
        return result;
    }

    // ── Level 2: Manufacturer ID (high confidence) ────────────
    if (hasFlockManufacturerId(manufacturerId)) {
        result.detected   = true;
        result.type       = FlockDeviceType::FlockCamera;
        result.confidence = 2;
        result.summary    = "Flock Safety device (manufacturer ID 0x09C8 / XUNTONG)";
        return result;
    }

    // ── Level 1a: Known OUI prefix ────────────────────────────
    if (hasFlockOUI(mac)) {
        result.detected    = true;
        result.type        = FlockDeviceType::FlockCamera;
        result.confidence  = 1;
        result.matchedOUI  = mac.substr(0, 8).c_str();
        result.summary     = "Flock Safety device (OUI match: " + result.matchedOUI + ")";
        return result;
    }

    // ── Level 1b: Device name keyword ─────────────────────────
    if (hasFlockName(name)) {
        result.detected     = true;
        result.confidence   = 1;
        result.matchedName  = name;

        if (name.indexOf("Raven") != -1 ||
            name.indexOf("ShotSpotter") != -1 ||
            name.indexOf("SoundThinking") != -1) {
            result.type    = FlockDeviceType::RavenGunshot;
            result.summary = "SoundThinking/ShotSpotter Raven (name match)";
        } else if (name.indexOf("Ext Battery") != -1 ||
                   name.indexOf("FS Ext") != -1) {
            result.type    = FlockDeviceType::FlockExtBattery;
            result.summary = "Flock Safety external battery unit";
        } else {
            result.type    = FlockDeviceType::FlockCamera;
            result.summary = "Flock Safety camera (name match: " + name + ")";
        }
        return result;
    }

    return result;  // not detected
}

// ============================================================
//  Logging
// ============================================================
void logDetection(const String& devTag, const FlockResult& result,
                  const String& address, int rssi) {
    if (!result.detected) return;

    const char* confidenceStr;
    switch (result.confidence) {
        case 3:  confidenceStr = "DEFINITIVE (Service UUID)"; break;
        case 2:  confidenceStr = "HIGH (Manufacturer ID)";    break;
        default: confidenceStr = "MODERATE (OUI / Name)";     break;
    }

    const char* typeStr;
    switch (result.type) {
        case FlockDeviceType::RavenGunshot:
            typeStr = "SoundThinking/ShotSpotter Raven (gunshot detector)"; break;
        case FlockDeviceType::FlockExtBattery:
            typeStr = "Flock Safety external battery unit"; break;
        default:
            typeStr = "Flock Safety ALPR camera"; break;
    }

    LOG(LOG_TARGET,
        devTag + "Surveillance device detected!\n"
        "   Type:       " + String(typeStr)        + "\n"
        "   Address:    " + address                + "\n"
        "   RSSI:       " + String(rssi)  + " dBm" + "\n"
        "   Confidence: " + String(confidenceStr)  + "\n"
        "   Detail:     " + result.summary);

    // Update stats
    stats.lastDetectionTime = millis();
    stats.lastDeviceName    = result.matchedName.isEmpty()
                              ? String(typeStr) : result.matchedName;

    if (result.type == FlockDeviceType::RavenGunshot) {
        stats.ravenDevicesFound++;
    } else {
        stats.flockCamerasFound++;
    }
}

// ============================================================
//  Stats
// ============================================================
String getStatsString() {
    String s = "Flock / Raven Detection Stats:\n";
    s += "   Flock cameras: " + String(stats.flockCamerasFound) + "\n";
    s += "   Raven units:   " + String(stats.ravenDevicesFound) + "\n";
    if (!stats.lastDeviceName.isEmpty())
        s += "   Last device:   " + stats.lastDeviceName + "\n";
    if (stats.lastDetectionTime > 0)
        s += "   Last seen:     "
           + String((millis() - stats.lastDetectionTime) / 1000) + "s ago";
    return s;
}

void resetStats() {
    stats = FlockStats{};
}

} // namespace FlockDetection
