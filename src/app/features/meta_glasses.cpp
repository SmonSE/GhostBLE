#include "meta_glasses.h"
#include "infrastructure/logging/logger.h"

namespace MetaGlasses {

// ── Stats instance ─────────────────────────────────────────────
MetaStats stats;

// ── Known name keywords (defined once here, not in every TU) ──
static const char* META_KEYWORDS[] = {
    "meta",
    "ray-ban",
    "rayban",
    "stories",      // Gen 1: "Ray-Ban Stories"
    "wayfarer",
    "headliner",
    "skyler",       // Model name
    "ferrari",      // Limited edition model
};
static constexpr size_t META_KEYWORD_COUNT =
    sizeof(META_KEYWORDS) / sizeof(META_KEYWORDS[0]);

// ── Detection ──────────────────────────────────────────────────

bool hasMetaGlassesName(const String& name) {
    if (name.isEmpty()) return false;

    String lower = name;
    lower.toLowerCase();

    for (size_t i = 0; i < META_KEYWORD_COUNT; i++) {
        if (lower.indexOf(META_KEYWORDS[i]) != -1) return true;
    }
    return false;
}

bool hasMetaManufacturerId(uint16_t manufacturerId) {
    return manufacturerId == META_MANUFACTURER_ID;
}

bool hasMetaServiceUUID(const NimBLEAdvertisedDevice* device) {
    if (!device) return false;

    int count = device->getServiceUUIDCount();
    for (int i = 0; i < count; i++) {
        NimBLEUUID uuid = device->getServiceUUID(i);
        if (uuid.equals(NimBLEUUID(META_SERVICE_UUID_FULL))) return true;

        // Also match 16-bit representation "fd5f"
        String s = uuid.toString().c_str();
        s.toLowerCase();
        if (s.indexOf("fd5f") != -1) return true;
    }
    return false;
}

bool hasMetaGATTService(NimBLEClient* pClient) {
    if (!pClient || !pClient->isConnected()) return false;
    return pClient->getService(META_SERVICE_UUID_FULL) != nullptr;
}

uint8_t detectMetaGlasses(const NimBLEAdvertisedDevice* device,
                           const String& name,
                           uint16_t manufacturerId) {
    // Level 3 — definitive: service UUID in advertisement
    if (hasMetaServiceUUID(device)) return 3;

    // Level 2 — high: registered Meta manufacturer ID
    if (hasMetaManufacturerId(manufacturerId)) return 2;

    // Level 1 — moderate: name keyword match only
    if (hasMetaGlassesName(name)) return 1;

    return 0;
}

// ── Classification ─────────────────────────────────────────────

String detectGeneration(const String& name, bool hasServiceUUID) {
    // Gen 2 (Meta Ray-Ban, 2023+) uses 0xFD5F service UUID
    if (hasServiceUUID) return "Gen 2";

    // Gen 1 (Ray-Ban Stories, 2021) did not advertise this UUID
    String lower = name;
    lower.toLowerCase();
    if (lower.indexOf("stories") != -1) return "Gen 1";

    return "Unknown";
}

String extractModelName(const String& name) {
    if (name.isEmpty()) return "Unknown Model";

    if (name.indexOf("Wayfarer")  != -1) return "Wayfarer";
    if (name.indexOf("Headliner") != -1) return "Headliner";
    if (name.indexOf("Skyler")    != -1) return "Skyler";
    if (name.indexOf("Ferrari")   != -1) return "Ferrari Edition";
    if (name.indexOf("Stories")   != -1) return "Stories (Gen 1)";

    return name;  // fallback: return full name
}

bool isHighValueTarget(uint8_t confidence) {
    // Only flag as high-value if manufacturer ID or service UUID confirmed —
    // name-only match is too weak to treat as a recording device warning.
    return confidence >= 2;
}

// ── Reporting ──────────────────────────────────────────────────

void logDetection(const String& devTag,
                  const String& address,
                  const String& name,
                  uint8_t       confidence,
                  const String& generation,
                  const String& model) {
    const char* confidenceStr;
    switch (confidence) {
        case 3:  confidenceStr = "DEFINITIVE (Service UUID 0xFD5F)"; break;
        case 2:  confidenceStr = "HIGH (Manufacturer ID 0x01AB)";    break;
        case 1:  confidenceStr = "MODERATE (Name match)";             break;
        default: confidenceStr = "NONE";                               break;
    }

    LOG(LOG_TARGET,
        devTag + "Meta Ray-Ban Glasses detected!\n"
        "   Address:    " + address    + "\n"
        "   Name:       " + name       + "\n"
        "   Model:      " + model      + "\n"
        "   Generation: " + generation + "\n"
        "   Confidence: " + String(confidenceStr));

    // Update stats
    stats.glassesFound++;
    stats.lastDetectionTime = millis();
    stats.lastModelDetected = model;

    if (generation == "Gen 2")     stats.gen2Detected++;
    else if (generation == "Gen 1") stats.gen1Detected++;
}

void processMetaGlasses(const NimBLEAdvertisedDevice* device,
                        const String& devTag,
                        const String& address,
                        const String& name,
                        uint16_t      manufacturerId) {
    uint8_t confidence = detectMetaGlasses(device, name, manufacturerId);
    if (confidence == 0) return;

    bool    hasUUID    = hasMetaServiceUUID(device);
    String  generation = detectGeneration(name, hasUUID);
    String  model      = extractModelName(name);

    logDetection(devTag, address, name, confidence, generation, model);
}

// ── Stats ──────────────────────────────────────────────────────

String getStatsString() {
    String s = "Meta Ray-Ban Stats:\n";
    s += "   Total found: " + String(stats.glassesFound)  + "\n";
    s += "   Gen 2:       " + String(stats.gen2Detected)  + "\n";
    s += "   Gen 1:       " + String(stats.gen1Detected)  + "\n";

    if (!stats.lastModelDetected.isEmpty())
        s += "   Last model:  " + stats.lastModelDetected + "\n";

    if (stats.lastDetectionTime > 0)
        s += "   Last seen:   "
           + String((millis() - stats.lastDetectionTime) / 1000) + "s ago";

    return s;
}

void resetStats() {
    stats = MetaStats{};
}

} // namespace MetaGlasses
