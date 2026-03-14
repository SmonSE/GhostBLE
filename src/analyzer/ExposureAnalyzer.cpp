#include "ExposureAnalyzer.h"
#include "../globals/globals.h"

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
    ExposureResult result{};
    int score = 0;

    result.exposureTier = determineExposureTier(dev);

    // -------- Device Type --------
    if (dev.manufacturer.find("Epson") != std::string::npos)
        result.deviceType = "Printer";
    else
        result.deviceType = "Unknown";

    bool isApple   = dev.manufacturer.find("Apple Inc.") != std::string::npos;
    bool isGoogle  = dev.manufacturer.find("Google") != std::string::npos;
    bool isSamsung = dev.manufacturer.find("Samsung") != std::string::npos;

    // -------- Exposure evaluation --------


    // Combine public and static MAC address reasons if both are true
    if (dev.isPublicMac && dev.hasStaticMac) {
        score += 40; // public
        score += 20; // static
        result.reasons.push_back("public static MAC address");
    } else if (dev.isPublicMac) {
        score += 40;
        result.reasons.push_back("public MAC address");
    } else if (dev.hasStaticMac) {
        score += 20;
        result.reasons.push_back("static MAC address");
    }

    if (dev.hasRotatingMac) {
        score -= 10;
        result.reasons.push_back("rotating MAC address");
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
        result.reasons.push_back("structured model info (reduced exposure)");
    }

    if (dev.gattHasIdentityInfo) {
        score -= 10;
        result.reasons.push_back("structured identity info (reduced exposure)");
    }

    bool genericAppleName =
        dev.name == "iPhone" ||
        dev.name.find("iPhone") != std::string::npos ||
        dev.name.find("iPad") != std::string::npos;

    if (genericAppleName)
        score -= 10;

    if (isApple || isGoogle || isSamsung) {
        result.reasons.push_back("modern mobile privacy implementation");
    }

    if (dev.isConnectable)
        score += 10;

    // -------- Exposure sources --------

    if (dev.gattHasEnvironmentName)
        score += 5;

    if (dev.advHasName)
        score += 12;

    if (dev.gattHasName)
        score += 5;

    if (dev.gattHasNameIdentityData)
        score += 10;

    if (dev.gattHasPersonalName)
        score += 30;

    // -------- Exposure Tier Impact --------
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

    // Apple devices typically expose identity later
    if (isApple)
        score -= 20;

    // -------- Minimal information mitigation --------
    // A public/static MAC alone enables tracking but reveals little about
    // the device or its owner.  Reduce the score when no other identifying
    // information is available so the rating reflects actual exposure.
    bool hasMinimalInfo = !dev.hasName && !dev.hasManufacturerData &&
                          !dev.advHasName && !dev.gattHasName &&
                          !dev.gattHasNameIdentityData && !dev.gattHasPersonalName &&
                          !dev.gattHasModelInfo && !dev.gattHasIdentityInfo &&
                          !dev.gattHasEnvironmentName;

    if (hasMinimalInfo && (dev.isPublicMac || dev.hasStaticMac)) {
        score -= 15;
        result.reasons.push_back("limited exposure (MAC only, no identifying data)");
    }

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
    else if ((dev.hasStaticMac || dev.isPublicMac) && !hasMinimalInfo)
        result.trackingRisk = "HIGH";
    else if (dev.hasStaticMac || dev.isPublicMac)
        result.trackingRisk = "MEDIUM";
    else
        result.trackingRisk = "MEDIUM";

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
}
