#pragma once

#include <Arduino.h>
#include <unordered_set>
#include <map>
#include <string>
#include <vector>
#include <atomic>

#include "app/gamification/xp_manager.h"


extern String devTag;
extern String localName;
extern String displayName;
extern String address;
extern String serviceInfo;
extern String manuInfo;
extern String payload;
extern String hexPayload;
extern String spacedPayload;

extern String deviceName;
extern String appearanceName;
extern String deviceInfoService;

extern String lastConnectedDeviceInfo;

extern const char index_html[] PROGMEM;

// Shared room words for environment name detection
extern const std::vector<std::string> roomWords;
