#include "flock_detection.h"
#include "infrastructure/logging/logger.h"

#include <ctype.h>

namespace FlockDetection {

// ===========================================================================
// Statistics
// ===========================================================================

FlockStats stats;


// ===========================================================================
// OUI DATABASE
//
// IMPORTANT:
//
// Flock-specific OUIs
// --------------------
// These are treated as strong evidence.
//
// Espressif OUIs
// ---------------
// These are NOT treated as Flock identification by themselves.
//
// An ESP32 can have the same OUI whether or not it is used by Flock.
// ===========================================================================


// ---------------------------------------------------------------------------
// Flock-specific OUIs
// ---------------------------------------------------------------------------

static const char* FLOCK_SPECIFIC_OUIS[] = {

    "10:02:b5",

    "50:02:91",

    "60:55:f9",

    "68:b6:b3",

    "7c:9e:bd",

    "90:38:0c",

    "a4:e5:7c",

    "c8:f0:9e",

    "e8:68:e7",

    "f0:08:d1"
};

static constexpr size_t FLOCK_SPECIFIC_OUI_COUNT =
    sizeof(FLOCK_SPECIFIC_OUIS) /
    sizeof(FLOCK_SPECIFIC_OUIS[0]);


// ---------------------------------------------------------------------------
// Espressif OUIs observed in Flock hardware
//
// These are only supporting evidence.
//
// DO NOT treat them as Flock identification.
// ---------------------------------------------------------------------------

static const char* FLOCK_ESPRESSIF_OUIS[] = {

    "18:fe:34",

    "24:0a:c4",

    "24:6f:28",

    "2c:f4:32",

    "30:ae:a4",

    "3c:61:05",

    "40:22:d8",

    "40:f5:20",

    "54:43:b2",

    "58:bf:25",

    "5c:cf:7f",

    "84:0d:8e",

    "84:cc:a8",

    "8c:aa:b5",

    "a4:cf:12",

    "b4:e6:2d",

    "cc:50:e3",

    "d8:a0:1d",

    "dc:54:75",

    "ec:62:60"
};

static constexpr size_t FLOCK_ESPRESSIF_OUI_COUNT =
    sizeof(FLOCK_ESPRESSIF_OUIS) /
    sizeof(FLOCK_ESPRESSIF_OUIS[0]);


// ===========================================================================
// Device names
// ===========================================================================

static const char* FLOCK_STRONG_NAMES[] = {

    "flock",
    "fs ext battery",
    "fs-ext",
    "pigvision"
};

static constexpr size_t FLOCK_STRONG_NAME_COUNT =
    sizeof(FLOCK_STRONG_NAMES) /
    sizeof(FLOCK_STRONG_NAMES[0]);


static const char* FLOCK_WEAK_NAMES[] = {

    "penguin"
};

static constexpr size_t FLOCK_WEAK_NAME_COUNT =
    sizeof(FLOCK_WEAK_NAMES) /
    sizeof(FLOCK_WEAK_NAMES[0]);


static const char* RAVEN_STRONG_NAMES[] = {

    "shotspotter",
    "soundthinking"
};

static constexpr size_t RAVEN_STRONG_NAME_COUNT =
    sizeof(RAVEN_STRONG_NAMES) /
    sizeof(RAVEN_STRONG_NAMES[0]);


static const char* RAVEN_WEAK_NAMES[] = {

    "raven"
};

static constexpr size_t RAVEN_WEAK_NAME_COUNT =
    sizeof(RAVEN_WEAK_NAMES) /
    sizeof(RAVEN_WEAK_NAMES[0]);


// ===========================================================================
// Manufacturer
// ===========================================================================
//
// 0x09C8 = XUNTONG
//
// This is associated with Flock hardware but is NOT treated as a
// definitive Flock identifier by itself.
// ===========================================================================

static constexpr uint16_t FLOCK_MANUFACTURER_ID = 0x09C8;


// ===========================================================================
// Raven UUIDs
// ===========================================================================


// ---------------------------------------------------------------------------
// Standard Bluetooth SIG services.
//
// These are deliberately weak evidence.
//
// 0x1809 = Health Thermometer
// 0x1819 = Location and Navigation
// ---------------------------------------------------------------------------

static const char* RAVEN_STANDARD_UUIDS[] = {

    "00001809",
    "00001819"
};

static constexpr size_t RAVEN_STANDARD_UUID_COUNT =
    sizeof(RAVEN_STANDARD_UUIDS) /
    sizeof(RAVEN_STANDARD_UUIDS[0]);


// ---------------------------------------------------------------------------
// Raven-specific services
// ---------------------------------------------------------------------------

static const char* RAVEN_SPECIFIC_UUIDS[] = {

    "00003100",   // GPS
    "00003200",   // Power
    "00003300",   // Network
    "00003400",   // Upload
    "00003500"    // Error
};

static constexpr size_t RAVEN_SPECIFIC_UUID_COUNT =
    sizeof(RAVEN_SPECIFIC_UUIDS) /
    sizeof(RAVEN_SPECIFIC_UUIDS[0]);


// ===========================================================================
// Internal helpers
// ===========================================================================

static std::string normalizeMac(const std::string& mac)
{
    std::string result = mac;

    for (char& c : result) {
        c = static_cast<char>(
            tolower(static_cast<unsigned char>(c))
        );
    }

    return result;
}


static std::string getOUI(const std::string& mac)
{
    if (mac.length() < 8) {
        return "";
    }

    return normalizeMac(mac.substr(0, 8));
}


static bool containsIgnoreCase(
    const String& value,
    const char* keyword)
{
    if (value.isEmpty() || keyword == nullptr) {
        return false;
    }

    String lower = value;
    lower.toLowerCase();

    String key = keyword;
    key.toLowerCase();

    return lower.indexOf(key) >= 0;
}


static bool uuidMatches(
    const std::string& uuid,
    const char* expected)
{
    if (expected == nullptr) {
        return false;
    }

    std::string normalized = uuid;

    for (char& c : normalized) {
        c = static_cast<char>(
            tolower(static_cast<unsigned char>(c))
        );
    }

    // Exact full UUID
    if (normalized == expected) {
        return true;
    }

    // NimBLE may return short UUID representations.
    //
    // Examples:
    //   00003100-0000-1000-8000-00805f9b34fb
    //   3100
    //
    String expectedStr = String(expected);

    if (normalized == expectedStr.substring(4).c_str()) {
        return true;
    }

    if (normalized.find(expected) == 0) {
        return true;
    }

    return false;
}


// ===========================================================================
// OUI checks
// ===========================================================================

bool hasFlockSpecificOUI(const std::string& mac)
{
    const std::string oui = getOUI(mac);

    if (oui.empty()) {
        return false;
    }

    for (size_t i = 0;
         i < FLOCK_SPECIFIC_OUI_COUNT;
         ++i) {

        if (oui == FLOCK_SPECIFIC_OUIS[i]) {
            return true;
        }
    }

    return false;
}


bool hasFlockEspressifOUI(const std::string& mac)
{
    const std::string oui = getOUI(mac);

    if (oui.empty()) {
        return false;
    }

    for (size_t i = 0;
         i < FLOCK_ESPRESSIF_OUI_COUNT;
         ++i) {

        if (oui == FLOCK_ESPRESSIF_OUIS[i]) {
            return true;
        }
    }

    return false;
}


// ---------------------------------------------------------------------------
// Backwards-compatible helper.
//
// IMPORTANT:
// This now intentionally excludes generic Espressif OUIs.
// ---------------------------------------------------------------------------

bool hasFlockOUI(const std::string& mac)
{
    return hasFlockSpecificOUI(mac);
}


// ===========================================================================
// Name detection
// ===========================================================================

bool hasFlockName(const String& name)
{
    if (name.isEmpty()) {
        return false;
    }

    for (size_t i = 0;
         i < FLOCK_STRONG_NAME_COUNT;
         ++i) {

        if (containsIgnoreCase(
                name,
                FLOCK_STRONG_NAMES[i])) {

            return true;
        }
    }

    return false;
}


bool hasRavenName(const String& name)
{
    if (name.isEmpty()) {
        return false;
    }

    for (size_t i = 0;
         i < RAVEN_STRONG_NAME_COUNT;
         ++i) {

        if (containsIgnoreCase(
                name,
                RAVEN_STRONG_NAMES[i])) {

            return true;
        }
    }

    return false;
}


// ===========================================================================
// Manufacturer
// ===========================================================================

bool hasFlockManufacturerId(uint16_t manufacturerId)
{
    return manufacturerId == FLOCK_MANUFACTURER_ID;
}


// ===========================================================================
// Raven UUID detection
// ===========================================================================

bool hasRavenServiceUUID(
    const NimBLEAdvertisedDevice* device,
    RavenFirmware& outFW)
{
    outFW = RavenFirmware::Unknown;

    if (!device) {
        return false;
    }

    const int svcCount =
        device->getServiceUUIDCount();

    int standardHits = 0;
    int specificHits = 0;

    for (int i = 0; i < svcCount; ++i) {

        std::string uuid =
            device->getServiceUUID(i).toString();

        // Standard Raven-associated services
        for (size_t j = 0;
             j < RAVEN_STANDARD_UUID_COUNT;
             ++j) {

            if (uuidMatches(
                    uuid,
                    RAVEN_STANDARD_UUIDS[j])) {

                standardHits++;
                break;
            }
        }

        // Raven-specific services
        for (size_t j = 0;
             j < RAVEN_SPECIFIC_UUID_COUNT;
             ++j) {

            if (uuidMatches(
                    uuid,
                    RAVEN_SPECIFIC_UUIDS[j])) {

                specificHits++;
                break;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Raven-specific UUIDs
    //
    // One specific UUID = interesting
    // Two or more = strong evidence
    // -----------------------------------------------------------------------

    if (specificHits >= 2) {

        outFW = RavenFirmware::V1_2_PLUS;

        return true;
    }

    // -----------------------------------------------------------------------
    // Standard services alone are NOT enough.
    //
    // They can occur on unrelated BLE devices.
    // -----------------------------------------------------------------------

    if (specificHits >= 1 &&
        standardHits >= 1) {

        outFW = RavenFirmware::V1_2_PLUS;

        return true;
    }

    return false;
}


// ===========================================================================
// Evidence helpers
// ===========================================================================

bool hasEvidence(
    const FlockResult& result,
    EvidenceFlags flag)
{
    return (result.evidence & flag) != 0;
}


String getConfidenceString(const FlockResult& result)
{
    if (result.state == DetectionState::Confirmed) {

        if (result.confidence >= 85) {
            return "VERY HIGH";
        }

        return "HIGH";
    }

    if (result.state == DetectionState::Suspected) {

        if (result.confidence >= 60) {
            return "MODERATE";
        }

        return "LOW";
    }

    return "NONE";
}


String getDetectionStateString(DetectionState state)
{
    switch (state) {

        case DetectionState::Confirmed:
            return "CONFIRMED";

        case DetectionState::Suspected:
            return "SUSPECTED";

        default:
            return "NONE";
    }
}


String getDeviceTypeString(FlockDeviceType type)
{
    switch (type) {

        case FlockDeviceType::FlockCamera:
            return "Flock Safety ALPR camera";

        case FlockDeviceType::FlockExtBattery:
            return "Flock Safety external battery";

        case FlockDeviceType::RavenGunshot:
            return "SoundThinking/ShotSpotter Raven";

        default:
            return "Unknown";
    }
}


String getEvidenceString(const FlockResult& result)
{
    String evidence;

    if (hasEvidence(
            result,
            EVIDENCE_FLOCK_OUI)) {

        evidence +=
            "Flock-specific OUI";
    }

    if (hasEvidence(
            result,
            EVIDENCE_ESPRESSIF_OUI)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Espressif OUI associated with Flock hardware";
    }

    if (hasEvidence(
            result,
            EVIDENCE_FLOCK_NAME)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Flock device name";
    }

    if (hasEvidence(
            result,
            EVIDENCE_RAVEN_NAME)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Raven device name";
    }

    if (hasEvidence(
            result,
            EVIDENCE_FLOCK_MANUFACTURER)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Manufacturer ID 0x09C8";
    }

    if (hasEvidence(
            result,
            EVIDENCE_RAVEN_STANDARD_UUID)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Raven-associated standard UUID";
    }

    if (hasEvidence(
            result,
            EVIDENCE_RAVEN_SPECIFIC_UUID)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Raven-specific service UUID";
    }

    if (hasEvidence(
            result,
            EVIDENCE_FLOCK_PAYLOAD)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "Flock advertisement payload";
    }

    if (hasEvidence(
            result,
            EVIDENCE_MULTI_SIGNAL)) {

        if (!evidence.isEmpty()) {
            evidence += ", ";
        }

        evidence +=
            "multiple independent signals";
    }

    if (evidence.isEmpty()) {
        evidence = "No strong evidence";
    }

    return evidence;
}


// ===========================================================================
// Main detection pipeline
// ===========================================================================

FlockResult detect(
    const NimBLEAdvertisedDevice* device,
    const String& name,
    uint16_t manufacturerId)
{
    FlockResult result;

    if (!device) {
        return result;
    }

    const std::string mac =
        device->getAddress().toString();

    const bool flockOUI =
        hasFlockSpecificOUI(mac);

    const bool espressifOUI =
        hasFlockEspressifOUI(mac);

    const bool flockName =
        hasFlockName(name);

    const bool ravenName =
        hasRavenName(name);

    const bool manufacturer =
        hasFlockManufacturerId(
            manufacturerId);

    RavenFirmware ravenFW =
        RavenFirmware::Unknown;

    const bool ravenUUID =
        hasRavenServiceUUID(
            device,
            ravenFW);


    // ========================================================================
    // Evidence collection
    // ========================================================================

    uint16_t score = 0;

    uint16_t evidence =
        EVIDENCE_NONE;


    // ------------------------------------------------------------------------
    // Flock-specific OUI
    // ------------------------------------------------------------------------

    if (flockOUI) {

        score += 40;

        evidence |=
            EVIDENCE_FLOCK_OUI;

        result.matchedOUI =
            mac.substr(0, 8).c_str();
    }


    // ------------------------------------------------------------------------
    // Espressif OUI
    //
    // Supporting evidence only.
    // ------------------------------------------------------------------------

    if (espressifOUI) {

        score += 10;

        evidence |=
            EVIDENCE_ESPRESSIF_OUI;

        result.matchedOUI =
            mac.substr(0, 8).c_str();
    }


    // ------------------------------------------------------------------------
    // Flock name
    // ------------------------------------------------------------------------

    if (flockName) {

        score += 40;

        evidence |=
            EVIDENCE_FLOCK_NAME;

        result.matchedName = name;
    }


    // ------------------------------------------------------------------------
    // Raven name
    // ------------------------------------------------------------------------

    if (ravenName) {

        score += 50;

        evidence |=
            EVIDENCE_RAVEN_NAME;

        result.matchedName = name;
    }


    // ------------------------------------------------------------------------
    // Manufacturer ID
    // ------------------------------------------------------------------------

    if (manufacturer) {

        score += 50;

        evidence |=
            EVIDENCE_FLOCK_MANUFACTURER;
    }


    // ------------------------------------------------------------------------
    // Raven UUID
    // ------------------------------------------------------------------------

    if (ravenUUID) {

        score += 65;

        evidence |=
            EVIDENCE_RAVEN_SPECIFIC_UUID;
    }


    // ========================================================================
    // Multiple independent signals
    // ========================================================================

    uint8_t signalCount = 0;

    if (flockOUI)       signalCount++;
    if (flockName)      signalCount++;
    if (ravenName)      signalCount++;
    if (manufacturer)   signalCount++;
    if (ravenUUID)      signalCount++;


    if (signalCount >= 2) {

        score += 15;

        evidence |=
            EVIDENCE_MULTI_SIGNAL;
    }


    // ========================================================================
    // No evidence
    // ========================================================================

    if (score == 0) {

        return result;
    }


    // ========================================================================
    // Determine device type
    // ========================================================================

    if (ravenUUID || ravenName) {

        result.type =
            FlockDeviceType::RavenGunshot;

        result.ravenFW =
            ravenFW;
    }
    else if (containsIgnoreCase(
                 name,
                 "fs ext battery") ||
             containsIgnoreCase(
                 name,
                 "fs-ext")) {

        result.type =
            FlockDeviceType::FlockExtBattery;
    }
    else {

        result.type =
            FlockDeviceType::FlockCamera;
    }


    // ========================================================================
    // Confidence / state
    // ========================================================================

    if (score > 100) {
        score = 100;
    }

    result.score =
        score;

    result.confidence =
        static_cast<uint8_t>(score);

    result.evidence =
        evidence;


    // ------------------------------------------------------------------------
    // Confirmed
    // ------------------------------------------------------------------------

    if (score >= 70) {

        result.state =
            DetectionState::Confirmed;

        result.detected = true;
    }

    // ------------------------------------------------------------------------
    // Suspected
    // ------------------------------------------------------------------------

    else if (score >= 30) {

        result.state =
            DetectionState::Suspected;

        result.detected = true;
    }

    // ------------------------------------------------------------------------
    // Below threshold
    // ------------------------------------------------------------------------

    else {

        // We deliberately don't report very weak matches.

        return result;
    }


    // ========================================================================
    // Summary
    // ========================================================================

    result.summary =
        getDeviceTypeString(result.type);

    if (result.ravenFW ==
        RavenFirmware::V1_2_PLUS) {

        result.summary +=
            " — Raven FW 1.2+ family";
    }

    result.summary +=
        " | Evidence: " +
        getEvidenceString(result);


    return result;
}


// ===========================================================================
// Logging
// ===========================================================================

void logDetection(
    const String& devTag,
    const FlockResult& result,
    const String& address,
    int rssi)
{
    if (!result.detected) {
        return;
    }


    const String state =
        getDetectionStateString(
            result.state);

    const String confidence =
        getConfidenceString(result);

    const String type =
        getDeviceTypeString(
            result.type);

    LOG(
        LOG_TARGET,

        devTag +
        "Surveillance device detected!\n"

        "   Type:       " +
        type +
        "\n"

        "   State:      " +
        state +
        "\n"

        "   Address:    " +
        address +
        "\n"

        "   RSSI:       " +
        String(rssi) +
        " dBm\n"

        "   Confidence: " +
        confidence +
        " (" +
        String(result.confidence) +
        "/100)\n"

        "   Evidence:   " +
        getEvidenceString(result) +
        "\n"

        "   Detail:     " +
        result.summary
    );


    // ========================================================================
    // Statistics
    // ========================================================================

    stats.lastDetectionTime =
        millis();

    stats.lastDeviceName =
        result.matchedName.isEmpty()
        ? type
        : result.matchedName;


    if (result.state ==
        DetectionState::Suspected) {

        stats.suspectedDevicesFound++;
    }


    if (result.type ==
        FlockDeviceType::RavenGunshot) {

        stats.ravenDevicesFound++;
    }
    else {

        stats.flockCamerasFound++;
    }
}


// ===========================================================================
// Statistics
// ===========================================================================

String getStatsString()
{
    String s =
        "Flock / Raven Detection Stats:\n";

    s +=
        "   Flock cameras: " +
        String(stats.flockCamerasFound) +
        "\n";

    s +=
        "   Raven units:   " +
        String(stats.ravenDevicesFound) +
        "\n";

    s +=
        "   Suspected:     " +
        String(stats.suspectedDevicesFound) +
        "\n";


    if (!stats.lastDeviceName.isEmpty()) {

        s +=
            "   Last device:   " +
            stats.lastDeviceName +
            "\n";
    }


    if (stats.lastDetectionTime > 0) {

        s +=
            "   Last seen:     " +
            String(
                (millis() -
                 stats.lastDetectionTime) /
                1000
            ) +
            "s ago";
    }


    return s;
}


// ===========================================================================
// Reset
// ===========================================================================

void resetStats()
{
    stats =
        FlockStats{};
}


} // namespace FlockDetection
