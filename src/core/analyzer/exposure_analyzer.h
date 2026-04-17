#pragma once
#include <string>
#include <vector>
#include "app/context/globals.h"
#include "core/models/device_info.h"

struct ExposureResult {
    std::string deviceType;
    std::string identityExposure;
    std::string trackingRisk;
    std::string privacyLevel;

    ExposureTier exposureTier = ExposureTier::None;

    std::vector<std::string> reasons;
};

ExposureResult analyzeExposure(const DeviceInfo& dev);
