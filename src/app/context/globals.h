#pragma once

#include <Arduino.h>
#include <unordered_set>
#include <map>
#include <string>
#include <vector>
#include <atomic>

#include "app/gamification/xp_manager.h"

extern XPManager xpManager;

// ScanDevices seenDevices (unordered_set for O(1) lookups instead of O(log n))
extern std::unordered_set<std::string> seenDevices;

// Device session ID tracking (MAC → incremental session ID)
extern std::atomic<int> nextDeviceSessionId;
extern std::map<std::string, int> deviceSessionMap;
int getOrAssignDeviceId(const std::string& mac);
extern std::vector<std::string> uuidList;
extern std::vector<std::string> nameList;

// Global flags and strings
extern bool isTarget;
extern bool targetFound;
extern std::atomic<bool> isGlassesTaskRunning;
extern std::atomic<bool> isAngryTaskRunning;
extern std::atomic<bool> isSadTaskRunning;
extern std::atomic<bool> isHappyTaskRunning;
extern std::atomic<bool> isThugLifeTaskRunning;
extern std::atomic<bool> isSpeechBubbleActive;
extern std::atomic<bool> isChargingState;

// FreeRTOS task handles for expression animations
extern TaskHandle_t glassesTaskHandle;
extern TaskHandle_t angryTaskHandle;
extern TaskHandle_t sadTaskHandle;
extern SemaphoreHandle_t taskMutex;
extern bool isWebLogActive;
extern bool is_connectable;
extern bool scanIsRunning;
extern bool bleScanEnabled;
extern bool wardrivingEnabled;
extern bool helpOverlayVisible;

extern std::atomic<int> pointer;
extern std::atomic<int> susDevice;
extern std::atomic<int> beaconsFound;
extern std::atomic<int> pwnbeaconsFound;
extern std::atomic<int> targetConnects;
extern std::atomic<int> allSpottedDevice;
extern std::atomic<int> leakedCounter;
extern std::atomic<int> batteryPercent;
extern std::atomic<int> riskScore;

// Security finding counters (reset per scan cycle)
extern std::atomic<int> highFindingsCount;
extern std::atomic<int> unencryptedSensitiveCount;
extern std::atomic<int> writableNoAuthCount;
extern std::atomic<int> rssi;

extern std::string addrStr;

extern unsigned long lastScanTime;
extern unsigned long lastFaceUpdate;

extern String devTag;
extern String localName;
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
