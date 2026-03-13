#pragma once
#include <set>
#include <string>
#include <Arduino.h>
#include <vector>
#include <atomic>

#include "../xp/XPManager.h"

extern XPManager xpManager;

// ScanDevices seenDevices
extern std::set<std::string> seenDevices;
extern std::vector<std::string> uuidList;
extern std::vector<std::string> nameList;

extern bool scanIsRunning;

// Global flags and strings
extern bool targetFound;
extern std::atomic<bool> isGlassesTaskRunning;
extern std::atomic<bool> isAngryTaskRunning;
extern std::atomic<bool> isSadTaskRunning;
extern std::atomic<bool> isHappyTaskRunning;
extern std::atomic<bool> isThugLifeTaskRunning;

// FreeRTOS task handles for expression animations
extern TaskHandle_t glassesTaskHandle;
extern TaskHandle_t angryTaskHandle;
extern TaskHandle_t sadTaskHandle;
extern SemaphoreHandle_t taskMutex;
extern bool isWebLogActive;
extern bool is_connectable;
extern bool bleScanEnabledWeb;
extern bool wardrivingEnabled;

extern std::atomic<int> susDevice;
extern std::atomic<int> beaconsFound;
extern std::atomic<int> targetConnects;
extern std::atomic<int> allSpottedDevice;
extern std::atomic<int> leakedCounter;
extern std::atomic<int> batteryPercent;
extern std::atomic<int> riskScore;
extern std::atomic<int> rssi;

extern std::string addrStr;

extern unsigned long lastScanTime;
extern unsigned long lastFaceUpdate;

extern String localName;
extern String address;
extern String serviceInfo;
extern String manuInfo;
extern String payload;
extern String hexPayload;
extern String spacedPayload;

extern String deviceInfoService;
extern String heartRateService;
extern String temperatureService;
extern String batteryLevelService;
extern String genericAccessService;
extern String timeInfoService;

extern String lastConnectedDeviceInfo;

extern const char index_html[] PROGMEM;

// Shared room words for environment name detection
extern const std::vector<std::string> roomWords;
