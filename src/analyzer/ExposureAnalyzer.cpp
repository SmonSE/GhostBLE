#include "ExposureAnalyzer.h"



static const std::vector<std::string> roomWords = {
    "wohnzimmer", "küche", "kueche", "bad",
    "schlafzimmer", "office", "living",
    "bedroom", "kitchen", "bath"
};

static ExposureTier determineExposureTier(const DeviceInfo& dev)
{
    // Tier 3 — Consent (personal or user-identifiable data)
    if (dev.gattHasPersonalName)
        return ExposureTier::Consent;

    // Tier 2 — Active (requires connection)
    if (dev.gattHasName || dev.gattHasNameIdentityData)
        return ExposureTier::Active;

    // Tier 1 — Passive (visible without connection)
    if (dev.advHasName)
        return ExposureTier::Passive;

    return ExposureTier::None;
}

ExposureResult analyzeExposure(const DeviceInfo& dev)
{
    ExposureResult result;

    int score = 0;

    result.exposureTier = determineExposureTier(dev);

    // -------- Device Type --------
    if (dev.manufacturer.find("Epson") != std::string::npos)
        result.deviceType = "Printer";
    else
        result.deviceType = "Unknown";

    // -------- Exposure evaluation --------
    if (dev.isPublicMac) {
        score += 40;
        result.reasons.push_back("public MAC address");
    }

    if (dev.hasStaticMac) {
        score += 20;
        result.reasons.push_back("static MAC");
    }

    if (dev.hasName) {
        score += 15;
        result.reasons.push_back("device name broadcasted");
    }

    if (dev.hasManufacturerData) {
        score += 10;
        result.reasons.push_back("manufacturer identifiable");
    }

    if (dev.gattHasModelInfo) {
        score -= 10;
        result.reasons.push_back("manufacturer identifiable");
    }

    if (dev.gattHasIdentityInfo) {
        score -= 10;
        result.reasons.push_back("manufacturer identifiable");
    }

    bool genericAppleName =
    dev.name == "iPhone" ||
    dev.name.find("iPhone") != std::string::npos ||
    dev.name.find("iPad") != std::string::npos;

    if (genericAppleName)
        score -= 10;

    bool isApple =
        dev.manufacturer.find("Apple Inc.") != std::string::npos;

    bool isGoogle =
        dev.manufacturer.find("Google") != std::string::npos;

    bool isSamsung =
        dev.manufacturer.find("Samsung") != std::string::npos;


    if (dev.hasRotatingMac) {
        score -= 10;
        result.reasons.push_back("rotating MAC address");
    }

    if (isApple || isGoogle || isSamsung) {
        result.reasons.push_back("modern mobile privacy implementation");
    }

    // Connectable devices expose more data and are more vulnerable to active attacks
    if (dev.isConnectable) {
        score += 10;
    }

    // Passive exposure (strong)
    if (dev.gattHasEnvironmentName)
        score += 5;

    // Passive exposure (strong)
    if (dev.advHasName)
        score += 12;

    if (dev.gattHasName)
        score += 5;    

    // Active exposure (moderate)
    if (dev.gattHasNameIdentityData)
        score += 10;

    // Personal name (strong even if GATT)
    if (dev.gattHasPersonalName)
        score += 30;

    // -------- Identity Exposure --------
    if (score > 60)
        result.identityExposure = "HIGH";
    else if (score > 30)
        result.identityExposure = "MEDIUM";
    else
        result.identityExposure = "LOW";

    // -------- Tracking Risk --------
    if (dev.hasRotatingMac)
        result.trackingRisk = "LOW";
    else if (dev.hasStaticMac || dev.isPublicMac)
        result.trackingRisk = "HIGH";
    else
        result.trackingRisk = "MEDIUM";

    // Apple devices expose identity only after connection
    if (isApple) {
        score -= 20;
    }    

    // -------- Privacy Level --------
    if (score > 80)
        result.privacyLevel = "VERY POOR";
    else if (score > 60)
        result.privacyLevel = "POOR";
    else if (score > 40)
        result.privacyLevel = "ACCEPTABLE";
    else if (score > 20)
        result.privacyLevel = "OK";
    else
        result.privacyLevel = "GOOD";

    return result;

    switch(result.exposureTier) {
        case ExposureTier::Passive:
            score += 30;
            result.reasons.push_back("identity exposed via advertising");
            break;

        case ExposureTier::Active:
            score += 10;
            result.reasons.push_back("identity readable after connection");
            break;

        case ExposureTier::Consent:
            score += 5;
            result.reasons.push_back("identity visible after interaction");
            break;

        default:
            break;
    }

}
