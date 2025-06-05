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
bool isThugLifeTaskRunning = false;
bool isWebLogActive = false;
bool is_connectable = false;

int susDevice = 0;
int targetConnects = 0;
int allSpottedDevice = 0;
int leakedCounter = 0;
int batteryPercent = 0;
int riskScore = 0;
int rssi = 0;

unsigned long lastScanTime = 0;
unsigned long lastFaceUpdate = 0;

String localName = "";
String address = "";
String serviceInfo = "";
String manuInfo = "";
String payload = "";
String hexPayload = "";
String spacedPayload = "";

String deviceInfoService = "";
String heartRateService = "";
String batteryLevelService = "";
String genericAccessService = "";
String timeInfoService = "";

String lastConnectedDeviceInfo = "Noch kein Gerät verbunden.";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>NibBLEs Logger</title>
  <style>
    body {
      background-color: black;
      color: #00FF00;
      font-family: monospace, monospace;
      font-size: 28px;
      margin: 10px;
    }
    h2 {
        font-size: 40px;
    }
    #log {
      white-space: pre-wrap;
    }
  </style>
</head>
  <body>
    <h2>NibBLEs Logger</h2>
    <pre id="log"></pre>
    <script>
      const logElement = document.getElementById('log');
      const socket = new WebSocket(`ws://${location.host}/ws`);

      socket.onopen = () => {
        logElement.textContent += "WebSocket connected\n";
      };

      socket.onmessage = (event) => {
        logElement.textContent += event.data + "\n";
      };

      socket.onclose = () => {
        logElement.textContent += "WebSocket disconnected\n";
      };
    </script>
  </body>
</html>
)rawliteral";
