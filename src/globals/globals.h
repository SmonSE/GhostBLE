#ifndef GLOBALS_H
#define GLOBALS_H

#pragma once
#include <set>
#include <string>
#include <Arduino.h>
#include <vector> 

// ScanDevices seenDevices
extern std::set<std::string> seenDevices;
extern std::vector<std::string> uuidList;
extern std::vector<std::string> nameList;

extern bool scanIsRunning;

// Global flags and strings
extern bool targetFound;
extern bool hasManuData;
extern bool skipLogging;
extern bool isGlassesTaskRunning;
extern bool isAngryTaskRunning;
extern bool isSadTaskRunning;
extern bool isHappyTaskRunning;
extern bool isThugLifeTaskRunning;
extern bool isWebLogActive;
extern bool is_connectable;
extern bool bleScanEnabledWeb;

extern int susDevice;
extern int beaconsFound;
extern int targetConnects;
extern int allSpottedDevice;
extern int leakedCounter;
extern int batteryPercent;
extern int riskScore;
extern int rssi;

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


#endif // GLOBALS_H
