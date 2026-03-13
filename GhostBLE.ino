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
#include "src/images/nibblesThugLife.h"

#include "src/logToSerialAndWeb/logger.h"
#include "src/gps/GPSManager.h"
#include "src/wardriving/WigleLogger.h"
#include "src/helper/nibblesSpeech.h"

#include <WiFi.h>
#include <AsyncTCP.h>


unsigned long startTimeDevice;
const unsigned long timerDurationDevice = 30 * 60 * 1000; // 30 minutes in milliseconds

const char* ap_ssid = WIFI_AP_SSID;
const char* ap_password = WIFI_AP_PASSWORD;

AsyncWebServer server(80);

auto keys = M5Cardputer.Keyboard.keysState();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client connected: %u\n", client->id());
  }
}

void playMysteryBoot() {
  M5.Speaker.setVolume(60);

  int notes[] = { 440, 622, 880, 740, 1046 };
  int durations[] = { 180, 120, 150, 220, 300 };

  for (int i = 0; i < 5; i++) {
    M5.Speaker.tone(notes[i], durations[i]);
    delay(durations[i] + 40);
  }

  // low bass finish
  M5.Speaker.tone(220, 400);
}

void playNotificationPro() {
  M5.Speaker.setVolume(40);

  int notes[] = { 1568, 1865, 2093 }; // G6, A#6, C7
  int dur = 80;

  for (int i = 0; i < 3; i++) {
    M5.Speaker.tone(notes[i], dur);
    delay(dur + 40);
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

// GPS and wardriving
GPSManager gpsManager;
WigleLogger wigleLogger;

File dataFile;
std::vector<String> serviceUuids;

void setup() {
  M5.Power.begin();
  M5Cardputer.begin();
  Serial.begin(115200);
  delay(500);

  Serial.println("GhostBLE starting...");

  initLogger();

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(BLACK);
  delay(250);

  NimBLEDevice::init("bleDefender");
  Serial.println("BLE initialized successfully.");

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  delay(200);

  // Deselect LoRa chip to free shared SPI bus for SD card
  #if defined(LORA_CS_PIN) && (LORA_CS_PIN >= 0)
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);
  #endif

  #if defined(CARDPUTER)
  if (!sdLogger.begin(SD_CS_PIN)) {
    drawThoughtBubble("NO SD CARD!", 125, 18);
    while (1);
  }
  #endif

  xpManager.begin();

  drawThoughtBubble("HI I'M NIBBLES", 125, 18);
  vTaskDelay(pdMS_TO_TICKS(2000));

  isWebLogActive = true; 
  startWebLogServer();

  ws.textAll("BLE_SCAN_READY");

  nibblesSpeechBegin();

  startTimeDevice = millis();
  scanIsRunning = false;

  delay(3000);

  toggleWiFi();

  // To Update Wifi Logo to ON
  showFindingCounter(targetConnects, susDevice, leakedCounter);
}


void loop() {
  ws.cleanupClients();  // Important for websocket memory management
  M5Cardputer.update();
  unsigned long currentTime = millis();

  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      auto status = M5Cardputer.Keyboard.keysState();

      if (status.enter) {
        Serial.println("ENTER pressed");
      } 
      if (status.fn) {    
        Serial.println("FN pressed");
        toggleWiFi();
      }
      if (status.tab){
        Serial.println("TAB pressed");
        toggleWardriving();
      }
      if (status.del){
        Serial.println("DEL pressed");
        switchGPSSource();
      }
    }
  }

  // Long press detection for BtnG0
  if (M5Cardputer.BtnA.isPressed()) {
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

  // Update GPS if wardriving is active
  if (wardrivingEnabled) {
    gpsManager.update();
  }

  // NibBLEs speech system (idle mumbling)
  nibblesSpeechUpdate(currentTime);

  // BLE scan loop
  if (bleScanEnabledWeb) {
    if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
      if (!targetFound && !scanIsRunning) {
        nibblesSpeechNotifyEvent();
        scanForDevices();
      } else {
        targetFound = false;
      }
      lastFaceUpdate = currentTime;
    }
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

void onLongPress() {
  bleScanEnabledWeb = !bleScanEnabledWeb;

  if (bleScanEnabledWeb) {
    logToSerialAndWeb("▶️ BLE Scan ENABLED");
    ws.textAll("BLE_SCAN_ON");
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);
    nibblesSpeechShow(SpeechContext::SCAN_START);
  }
  else {
    logToSerialAndWeb("⏹️ BLE Scan DISABLED");
    ws.textAll("BLE_SCAN_OFF");
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice); 
    stopBleScan();   // THIS is the important part
  }
}

void toggleWiFi() {
  if (wifiStarted) {
    logToSerialAndWeb("WIFI / WEB SERVER OFF");
    stopWebLogServer();
    wifiStarted = false;
    isWebLogActive = false;
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, leakedCounter); // optional: Icon OFF
  } else {
    logToSerialAndWeb("WIFI / WEB SERVER ON");
    startWebLogServer();
    isWebLogActive = true;
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, leakedCounter); // optional: Icon ON
  }

}

void stopWebLogServer() {
  ws.closeAll();       // disconnect Clients
  server.end();        // stop server
  delay(50); 
  WiFi.softAPdisconnect(true);  // Access Point beenden
  delay(50);

  WiFi.mode(WIFI_OFF);
  delay(100);                   // THIS is critical

  wifiStarted    = false;
  isWebLogActive = false;

  Serial.println("WiFi fully stopped");
}


void startWebLogServer() {
  logToSerialAndWeb("WEB SERVER");
  if (wifiStarted) return;  // Don't start twice
  WiFi.mode(WIFI_AP);
  Serial.print("   - SoftAP started? YES\n");
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("   - Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // Setup WebSocket events and handlers BEFORE starting the server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
  logToSerialAndWeb("   - Web server started.");
  wifiStarted = true;

  logToSerialAndWeb("Pres BtnG0 to TOGGLE BLE Scan");
}

void toggleWardriving() {
  wardrivingEnabled = !wardrivingEnabled;

  if (wardrivingEnabled) {
    gpsManager.begin(GPSSource::GROVE);
    wigleLogger.begin();
    logToSerialAndWeb("Wardriving ON (" + String(gpsManager.getSourceName()) + ")");
    logToSerialAndWeb("  File: " + wigleLogger.getFilename());
  } else {
    wigleLogger.end();
    logToSerialAndWeb("Wardriving OFF (" + String(wigleLogger.getLoggedCount()) + " logged)");
  }

  ws.textAll(wardrivingEnabled ? "WARDRIVE_ON" : "WARDRIVE_OFF");
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);
  if (wardrivingEnabled) {
    nibblesSpeechShow(SpeechContext::WARDRIVING);
  }
}

void switchGPSSource() {
  if (!wardrivingEnabled) {
    logToSerialAndWeb("Enable wardriving first (TAB)");
    return;
  }

  GPSSource next = (gpsManager.getSource() == GPSSource::GROVE)
                   ? GPSSource::LORA_CAP
                   : GPSSource::GROVE;
  gpsManager.switchSource(next);
  logToSerialAndWeb("GPS: " + String(gpsManager.getSourceName()));

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);
}

