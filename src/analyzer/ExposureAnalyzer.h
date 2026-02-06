#pragma once
#include <string>
#include <vector>
#include "../models/DeviceInfo.h"

struct ExposureResult {
    std::string deviceType;
    std::string identityExposure;
    std::string trackingRisk;
    std::string privacyLevel;

    ExposureTier exposureTier = ExposureTier::None;

    std::vector<std::string> reasons;
};

ExposureResult analyzeExposure(const DeviceInfo& dev);
