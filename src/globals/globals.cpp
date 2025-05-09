#include "globals.h"
#include <set>

std::set<std::string> seenDevices;

bool scanIsRunning = false;

bool targetFound = false;
bool hasManuData = false;
bool skipLogging = false;
bool isGlassesTaskRunning = false;
bool isAngryTaskRunning = false;
bool isSadTaskRunning = false;

int susDevice = 0;
int targetConnects = 0;
int spottedDevice = 0;

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

