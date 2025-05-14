#include "globals.h"
#include <set>
#include <string>
#include <vector> 

std::set<std::string> seenDevices;
std::vector<std::string> uuidList;
std::vector<std::string> nameList;

bool scanIsRunning = false;

bool targetFound = false;
bool hasManuData = false;
bool skipLogging = false;
bool isGlassesTaskRunning = false;
bool isAngryTaskRunning = false;
bool isSadTaskRunning = false;

int susDevice = 0;
int targetConnects = 0;
int allSpottedDevice = 0;
int batteryPercent = 0;

unsigned long lastScanTime = 0;
unsigned long lastFaceUpdate = 0;

String localName = "";
String address = "";
String serviceInfo = "";

String deviceInfoService = "";
String heartRateService = "";
String batteryLevelService = "";
String timeInfoService = "";

String lastConnectedDeviceInfo = "Noch kein Gerät verbunden.";

