#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// Global flags and strings
extern bool targetFound;
extern bool hasManuData;
extern bool skipLogging;
extern bool isGlassesTaskRunning;
extern bool isAngryTaskRunning;

extern int targetFoundCount;

extern unsigned long lastScanTime;
extern unsigned long lastFaceUpdate;

// Filterlogik: Herstellerdaten prüfen
extern bool skipLogging;

extern String manuInfo;
extern String targetMessage;
extern String mainUuidStr;
extern String localName;
extern String address;
extern String serviceInfo;

extern String deviceInfoService;
extern String genericAccessInfo;
extern String heartRateService;
extern String batteryLevelService;
extern String timeInfoService;

extern String lastConnectedDeviceInfo;


#endif // GLOBALS_H
