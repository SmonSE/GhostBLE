#include "exposure_analyzer.h"


ExposureTier determineExposureTier(const DeviceInfo& dev)
{
    if (dev.gattHasPersonalName)
        return ExposureTier::Consent;

    if (dev.gattHasName || dev.gattHasNameIdentityData)
        return ExposureTier::Active;

    if (dev.advHasName)
        return ExposureTier::Passive;

    return ExposureTier::None;
}


void classifyDevice(const DeviceInfo& dev, ExposureResult& result)
{
    if (dev.manufacturer.find("Epson") != std::string::npos)
        result.deviceType = "Printer";
    else
        result.deviceType = "Unknown";
}


int calculateExposureScore(const DeviceInfo& dev, ExposureResult& result)
{
    int score = 0;

    bool isApple   = dev.manufacturer.find("Apple Inc.") != std::string::npos;
    bool isGoogle  = dev.manufacturer.find("Google") != std::string::npos;
    bool isSamsung = dev.manufacturer.find("Samsung") != std::string::npos;

    // -------- MAC behavior --------
    if (dev.isPublicMac && dev.hasStaticMac) {
        score += 60;
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

    // -------- Identity exposure --------
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

    // -------- Device behavior --------
    if (dev.isConnectable)
        score += 10;

    // -------- Security factors --------
    if (dev.hasDFUService) {
        score += 20;
        result.reasons.push_back("firmware update service exposed (DFU)");
    }

    if (dev.hasUARTService) {
        score += 15;
        result.reasons.push_back("serial/UART service exposed");
    }

    if (dev.hasNotifyData) {
        int notifyScore = 25;
        // More characteristics streaming = higher exposure
        if (dev.notifyCharCount > 2) notifyScore += 10;
        score += notifyScore;
        result.reasons.push_back("live sensor data streamed without authentication ("
            + std::to_string(dev.notifyCharCount) + " characteristic(s))");
    }

    if (dev.hasIndicateData) {
        score += 15;
        result.reasons.push_back("medical-grade indications received (acknowledged notify)");
    }

    if (dev.hasWritableChars && !dev.connectionEncrypted) {
        score += 15;
        result.reasons.push_back("writable characteristics without encryption");
    }

    if (dev.hasSensitiveUnencrypted) {
        score += 20;
        result.reasons.push_back("sensitive service without encryption");
    }

    if (dev.supportsBrEdr) {
        score += 5;
        result.reasons.push_back("dual-mode device (BLE + Classic)");
    }

    // -------- Vendor adjustments --------
    if (isApple)
        score -= 20;

    if (isApple || isGoogle || isSamsung) {
        result.reasons.push_back("modern mobile privacy implementation");
    }

    // -------- Minimal info mitigation --------
    bool hasMinimalInfo =
        !dev.hasName &&
        !dev.hasManufacturerData &&
        !dev.advHasName &&
        !dev.gattHasName &&
        !dev.gattHasNameIdentityData &&
        !dev.gattHasPersonalName &&
        !dev.gattHasModelInfo &&
        !dev.gattHasIdentityInfo &&
        !dev.gattHasEnvironmentName;

    if (hasMinimalInfo && (dev.isPublicMac || dev.hasStaticMac)) {
        score -= 15;
        result.reasons.push_back("limited exposure (MAC only, no identifying data)");
    }

    return score;
}


void finalizeExposureRatings(const DeviceInfo& dev, ExposureResult& result)
{
    // -------- Identity Exposure --------
    if (result.score > 60)
        result.identityExposure = "HIGH";
    else if (result.score > 30)
        result.identityExposure = "MEDIUM";
    else
        result.identityExposure = "LOW";

    // -------- Tracking Risk --------
    bool hasMinimalInfo =
        !dev.hasName &&
        !dev.hasManufacturerData &&
        !dev.advHasName &&
        !dev.gattHasName &&
        !dev.gattHasNameIdentityData &&
        !dev.gattHasPersonalName;

    if (dev.hasRotatingMac)
        result.trackingRisk = "LOW";
    else if (dev.hasNotifyData && (dev.hasStaticMac || dev.isPublicMac) && !hasMinimalInfo)
        result.trackingRisk = "HIGH";
    else if (dev.hasStaticMac || dev.isPublicMac)
        result.trackingRisk = "MEDIUM";
    else
        result.trackingRisk = "MEDIUM";

    // -------- Privacy Level --------
    if (result.score > 80)
        result.privacyLevel = "VERY POOR";
    else if (result.score > 60)
        result.privacyLevel = "POOR";
    else if (result.score > 40)
        result.privacyLevel = "ACCEPTABLE";
    else if (result.score > 20)
        result.privacyLevel = "OK";
    else
        result.privacyLevel = "GOOD";
}


ExposureResult analyzeExposure(const DeviceInfo& dev)
{
    ExposureResult result{};

    result.exposureTier = determineExposureTier(dev);

    classifyDevice(dev, result);

    result.score = calculateExposureScore(dev, result);

    // Tier impact
    switch (result.exposureTier) {
        case ExposureTier::Passive:
            result.score += 30;
            result.reasons.push_back("identity exposed via advertising");
            break;

        case ExposureTier::Active:
            result.score += 10;
            result.reasons.push_back("identity readable after connection");
            break;

        case ExposureTier::Consent:
            result.score += 5;
            result.reasons.push_back("identity visible after interaction");
            break;

        default:
            break;
    }

    finalizeExposureRatings(dev, result);

    return result;
}
