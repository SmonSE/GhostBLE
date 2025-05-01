#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>

// Global flags and strings
extern bool deviceFound;
extern bool hasManuData;

// Filterlogik: Herstellerdaten prüfen
extern bool skipLogging;

extern String manuInfo;
extern String targetMessage;
extern String mainUuidStr;
extern String localName;
extern String address;

extern String deviceInfoService;
extern String heartRateService;
extern String batteryLevelService;

#endif // GLOBALS_H
