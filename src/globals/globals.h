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

extern bool scanIsRunning;

// Global flags and strings
extern bool targetFound;
extern bool hasManuData;
extern bool skipLogging;
extern bool isGlassesTaskRunning;
extern bool isAngryTaskRunning;
extern bool isSadTaskRunning;

extern int susDevice;
extern int targetConnects;
extern int allSpottedDevice;

extern unsigned long lastScanTime;
extern unsigned long lastFaceUpdate;

extern String targetMessage;
extern String localName;
extern String address;
extern String serviceInfo;

extern String deviceInfoService;
extern String heartRateService;
extern String batteryLevelService;
extern String timeInfoService;

extern String lastConnectedDeviceInfo;


#endif // GLOBALS_H
