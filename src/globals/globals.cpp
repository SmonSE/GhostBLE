#include "globals.h"
#include <set>
#include <map>
#include <string>
#include <vector>
#include <atomic>

XPManager xpManager;

std::set<std::string> seenDevices;

std::atomic<int> nextDeviceSessionId{1};
std::map<std::string, int> deviceSessionMap;

int getOrAssignDeviceId(const std::string& mac) {
    auto it = deviceSessionMap.find(mac);
    if (it != deviceSessionMap.end()) return it->second;
    int id = nextDeviceSessionId++;
    deviceSessionMap[mac] = id;
    return id;
}
std::vector<std::string> uuidList;
std::vector<std::string> nameList;

bool scanIsRunning = false;

bool targetFound = false;
std::atomic<bool> isGlassesTaskRunning{false};
std::atomic<bool> isAngryTaskRunning{false};
std::atomic<bool> isSadTaskRunning{false};
std::atomic<bool> isHappyTaskRunning{false};
std::atomic<bool> isThugLifeTaskRunning{false};

TaskHandle_t glassesTaskHandle = NULL;
TaskHandle_t angryTaskHandle = NULL;
TaskHandle_t sadTaskHandle = NULL;
SemaphoreHandle_t taskMutex = NULL;

bool isWebLogActive = false;
bool is_connectable = false;
bool bleScanEnabledWeb = false;
bool wardrivingEnabled = false;

std::atomic<int> susDevice{0};
std::atomic<int> beaconsFound{0};
std::atomic<int> pwnbeaconsFound{0};
std::atomic<int> targetConnects{0};
std::atomic<int> allSpottedDevice{0};
std::atomic<int> leakedCounter{0};
std::atomic<int> batteryPercent{0};
std::atomic<int> riskScore{0};
std::atomic<int> rssi{0};

std::string addrStr = "";

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
String temperatureService = "";
String currentTimeService = "";
String batteryLevelService = "";
String genericAccessService = "";
String timeInfoService = "";

String lastConnectedDeviceInfo = "Noch kein Gerät verbunden.";

const std::vector<std::string> roomWords = {
    "wohnzimmer", "küche", "kueche", "bad",
    "schlafzimmer", "office", "living",
    "bedroom", "kitchen", "bath",
    "arbeitszimmer", "gästezimmer", "garage", "büro"
};

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
        logElement.textContent += "WebSocket connected. Pres BtnG0 to TOGGLE BLE Scan\n";
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
