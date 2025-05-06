#include "globals.h"

bool targetFound = false;
bool hasManuData = false;
bool skipLogging = false;
bool isGlassesTaskRunning = false;
bool isAngryTaskRunning = false;

int targetFoundCount = 0;

unsigned long lastScanTime = 0;
unsigned long lastFaceUpdate = 0;

String manuInfo = "";
String targetMessage = "";
String mainUuidStr = "";
String localName = "";
String address = "";
String serviceInfo = "";

String deviceInfoService = "";
String genericAccessInfo = "";
String heartRateService = "";
String batteryLevelService = "";
String timeInfoService = "";

String lastConnectedDeviceInfo = "Noch kein Gerät verbunden.";

