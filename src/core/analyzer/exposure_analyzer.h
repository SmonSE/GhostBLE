#pragma once

#include <string>
#include <vector>
#include "core/models/device_info.h"


struct ExposureResult {
    std::string deviceType;
    std::string identityExposure;
    std::string trackingRisk;
    std::string privacyLevel;

    ExposureTier exposureTier = ExposureTier::None;

    std::vector<std::string> reasons;

    int score = 0;
};


// Main function
ExposureResult analyzeExposure(const DeviceInfo& dev);

// -------------------- TESTABLE UNITS --------------------
ExposureTier determineExposureTier(const DeviceInfo& dev);

void classifyDevice(const DeviceInfo& dev, ExposureResult& result);
int calculateExposureScore(const DeviceInfo& dev, ExposureResult& result);
void finalizeExposureRatings(const DeviceInfo& dev, ExposureResult& result);