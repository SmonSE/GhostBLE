#include "globals.h"
#include <unordered_set>
#include <map>
#include <string>
#include <vector>
#include <atomic>

XPManager xpManager;

std::unordered_set<std::string> seenDevices;

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



bool isTarget = false;
bool targetFound = false;
std::atomic<bool> isGlassesTaskRunning{false};
std::atomic<bool> isAngryTaskRunning{false};
std::atomic<bool> isSadTaskRunning{false};
std::atomic<bool> isHappyTaskRunning{false};
std::atomic<bool> isThugLifeTaskRunning{false};
std::atomic<bool> isSpeechBubbleActive{false};
std::atomic<bool> isChargingState{false};

TaskHandle_t glassesTaskHandle = NULL;
TaskHandle_t angryTaskHandle = NULL;
TaskHandle_t sadTaskHandle = NULL;
SemaphoreHandle_t taskMutex = NULL;

bool isWebLogActive = false;
bool is_connectable = false;
bool scanIsRunning = false;
bool bleScanEnabled = false;
bool wardrivingEnabled = false;
bool helpOverlayVisible = false;

std::atomic<int> susDevice{0};
std::atomic<int> beaconsFound{0};
std::atomic<int> pwnbeaconsFound{0};
std::atomic<int> targetConnects{0};
std::atomic<int> allSpottedDevice{0};
std::atomic<int> leakedCounter{0};
std::atomic<int> batteryPercent{0};
std::atomic<int> riskScore{0};

std::atomic<int> highFindingsCount{0};
std::atomic<int> unencryptedSensitiveCount{0};
std::atomic<int> writableNoAuthCount{0};
std::atomic<int> rssi{0};

std::string addrStr = "";

unsigned long lastScanTime = 0;
unsigned long lastFaceUpdate = 0;


String devTag = "";
String localName = "";
String address = "";
String serviceInfo = "";
String manuInfo = "";
String payload = "";
String hexPayload = "";
String spacedPayload = "";

String deviceName = "";
String appearanceName = "";
String deviceInfoService = "";

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
  <meta charset="UTF-8">
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
    .settings {
      margin: 20px 0;
      padding: 10px;
      border: 1px solid #00FF00;
    }
    .settings input {
      background: black;
      color: #00FF00;
      border: 1px solid #00FF00;
      font-family: monospace;
      font-size: 20px;
      padding: 4px;
      margin: 4px;
    }
    .settings button {
      background: #00FF00;
      color: black;
      border: none;
      font-family: monospace;
      font-size: 20px;
      padding: 4px 12px;
      cursor: pointer;
      margin: 4px;
    }
    .settings label {
      display: inline-block;
      width: 120px;
      font-size: 20px;
    }
    .settings div { margin: 6px 0; }
    .status { color: #888; font-size: 16px; }
  </style>
</head>
  <body>
    <h2>NibBLEs Logger</h2>
    <div class="settings">
      <b>Settings (reboot to apply)</b>
      <div><label>Name:</label><input id="cfg-name" maxlength="20" placeholder="NibBLEs"><button onclick="send('NAME')">Set</button></div>
      <div><label>Face:</label><input id="cfg-face" maxlength="20" placeholder="(◕‿◕)"><button onclick="send('FACE')">Set</button></div>
      <div><label>WiFi SSID:</label><input id="cfg-ssid" maxlength="32" placeholder="GhostBLE"><button onclick="send('SSID')">Set</button></div>
      <div><label>WiFi Pass:</label><input id="cfg-pass" type="password" maxlength="63" placeholder="********"><button onclick="send('PASS')">Set</button></div>
      <div class="status" id="cfg-status"></div>
    </div>
    <pre id="log"></pre>
    <script>
      const logElement = document.getElementById('log');
      const statusEl = document.getElementById('cfg-status');
      const socket = new WebSocket(`ws://${location.host}/ws`);

      socket.onopen = () => {
        logElement.textContent += "WebSocket connected. Press BtnG0 to TOGGLE BLE Scan\n";
        socket.send('GET_CONFIG');
      };

      socket.onmessage = (event) => {
        if (event.data.startsWith('CONFIG:')) {
          const parts = event.data.substring(7).split('\t');
          if (parts[0]) document.getElementById('cfg-name').value = parts[0];
          if (parts[1]) document.getElementById('cfg-face').value = parts[1];
          if (parts[2]) document.getElementById('cfg-ssid').value = parts[2];
        } else if (event.data.includes('_SET:')) {
          statusEl.textContent = 'Saved! Reboot to apply.';
        } else {
          logElement.textContent += event.data + "\n";
        }
      };

      socket.onclose = () => {
        logElement.textContent += "WebSocket disconnected\n";
      };

      function send(key) {
        const val = document.getElementById('cfg-' + key.toLowerCase()).value;
        if (val) {
          socket.send('SET_' + key + ':' + val);
          statusEl.textContent = 'Sending...';
        }
      }
    </script>
  </body>
</html>
)rawliteral";
