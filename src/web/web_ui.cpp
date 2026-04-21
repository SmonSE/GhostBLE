#include "web_ui.h"

// ------------------------------------------------------------
//  Raumwörter
// ------------------------------------------------------------
const std::vector<std::string> roomWords = {
    "wohnzimmer", "küche", "kueche", "bad",
    "schlafzimmer", "office", "living",
    "bedroom", "kitchen", "bath",
    "arbeitszimmer", "gästezimmer", "garage", "büro"
};

// ------------------------------------------------------------
//  WebSocket-Dashboard
//  Konfigurierbare Felder: Name, Face, WiFi-SSID, WiFi-Passwort
//  Änderungen werden per WebSocket an deviceConfig übergeben
//  und erst nach Reboot aktiv (NVS-Persistenz).
// ------------------------------------------------------------
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
    h2 { font-size: 40px; }
    #log { white-space: pre-wrap; }
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
    const statusEl   = document.getElementById('cfg-status');
    const socket     = new WebSocket(`ws://${location.host}/ws`);

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
