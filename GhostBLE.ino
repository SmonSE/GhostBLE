#include <M5Cardputer.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include <NimBLEDevice.h>

#include "src/globals/globals.h"
#include "src/config/config.h"
#include "src/sdCard/SDLogger.h"
#include "src/scanner/ScanDevices.h"
#include "src/helper/drawOverlay.h"
#include "src/helper/showExpression.h"

#include "src/images/nibblesStartWorking.h"
#include "src/images/nibblesFront.h"
#include "src/images/nibblesGlasses.h"
#include "src/images/nibblesAngry.h"
#include "src/images/nibblesSad.h"
#include "src/images/nibblesHeartLeft.h"
#include "src/images/nibblesHeartRight.h"
#include "src/images/nibblesHappy.h"

#include "src/logToSerialAndWeb/logger.h"

#include <WiFi.h>
#include <AsyncTCP.h>


unsigned long startTimeDevice;
const unsigned long timerDurationDevice = 60 * 60 * 1000; // 60 Minuten in Millisekunden

const char* ap_ssid = "ESP32-Log";
const char* ap_password = "12345678";

AsyncWebServer server(80);

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client connected: %u\n", client->id());
  }
}

// Forward declaration
void onLongPress();

// Button long press tracking
unsigned long buttonPressStart = 0;
bool buttonHeld = false;
bool wifiStarted = false;

// External global instances
extern SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;

void setup() {
  M5Cardputer.begin();
  Serial.begin(115200);
  delay(500);

  Serial.println("GhostBLE starting...");

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawBmp(nibblesStartWorking, sizeof(nibblesStartWorking));
  delay(5000);

  NimBLEDevice::init("bleDefender");

  #if defined(CARDPUTER)
  if (!sdLogger.begin(SD_CS_PIN)) {
    while (1);
  }
  #endif

  Serial.println("BLE initialized successfully.");

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);

  showFindingCounter(targetConnects, susDevice, allSpottedDevice);

  delay(1000);

  // **Hier den Webserver und AP direkt starten**
  isWebLogActive = true;     // Status anpassen
  startWebLogServer();

  startTimeDevice = millis();
  scanIsRunning = false;
}


void loop() {
  ws.cleanupClients();  // Wichtig für WebSocket-Verbindungen
  M5Cardputer.update();
  unsigned long currentTime = millis();

  // Long press detection for BtnG0
  if (M5Cardputer.BtnA.isPressed()) {
    Serial.println("### BUTTON BTN0 PRESSED ###");
    if (!buttonHeld) {
      if (buttonPressStart == 0) {
        buttonPressStart = currentTime;
      } else if (currentTime - buttonPressStart >= 1000) {  // 1 second
        buttonHeld = true;
        onLongPress();  // Call your custom function
      }
    }
  } else {
    buttonPressStart = 0;
    buttonHeld = false;
  }

  // BLE scan loop
  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (!targetFound && !scanIsRunning) {
      NimBLEDevice::deinit(true);
      NimBLEDevice::init("");
      scanForDevices();
    } else {
      targetFound = false;
    }
    lastFaceUpdate = currentTime;
  }

  // Timer every 60 minutes
  unsigned long currentTimeDevice = millis();
  if (currentTimeDevice - startTimeDevice >= timerDurationDevice) {
    Serial.println("60 Minuten sind vorbei!");
    if (!seenDevices.empty()) {
      seenDevices.clear();
      Serial.println("CLEAR SEEN DEVICES");
    } else {
      Serial.println("SEEN DEVICES STILL EMPTY");
    }
    Serial.println("Update Batterie State");
    startTimeDevice = millis();
  }

  // Let system handle BLE, GPIO, etc.
  yield();
}

void stopWebLogServer() {
  ws.closeAll();       // alle Clients trennen
  server.end();        // Server stoppen
  WiFi.softAPdisconnect(true);  // Access Point beenden
}

void onLongPress() {
  isWebLogActive = !isWebLogActive;  // Toggle status
  if (isWebLogActive) {
    Serial.println("Long press detected: Web server OFF");
    logToSerialAndWeb("Web server stopped.");
    stopWebLogServer();
  }
}

void startWebLogServer() {
  logToSerialAndWeb("WEB SERVER");

  if (wifiStarted) return;  // Don't start twice

  WiFi.softAP(ap_ssid, ap_password);
  logToSerialAndWeb("   SoftAP started? YES");
  logToSerialAndWeb("   Access Point IP: " + WiFi.softAPIP());

  // Setup WebSocket events and handlers BEFORE starting the server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
  logToSerialAndWeb("   Web server started.");
  wifiStarted = true;
}
