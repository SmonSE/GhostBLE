#include <M5Cardputer.h>
#include <M5Unified.h>
#include <SPI.h>
#include <vector>
#include <unordered_set>
#include <NimBLEDevice.h>

#include "src/globals/globals.h"
#include "src/config/config.h"
#include "src/scanner/ScanDevices.h"
#include "src/helper/drawOverlay.h"
#include "src/helper/showExpression.h"
#include "src/helper/screenshot.h"

#include "src/images/nibblesStartWorking.h"
#include "src/images/nibblesFront.h"
#include "src/images/nibblesGlasses.h"
#include "src/images/nibblesAngry.h"
#include "src/images/nibblesSad.h"
#include "src/images/nibblesHeartLeft.h"
#include "src/images/nibblesHeartRight.h"
#include "src/images/nibblesHappy.h"
#include "src/images/nibblesHappyLeft.h"
#include "src/images/nibblesThugLife.h"

#include "src/logger/logger.h"
#include "src/GATTServices/pwnBeaconService.h"
#include "src/GATTServices/GATTServiceInit.h"
#include "src/gps/GPSManager.h"
#include "src/wardriving/WigleLogger.h"
#include "src/helper/nibblesSpeech.h"

#include <WiFi.h>
#include <AsyncTCP.h>


// Reactive memory cleanup uses heap threshold + set size (see loop)

const char* ap_ssid = WIFI_AP_SSID;
const char* ap_password = WIFI_AP_PASSWORD;

AsyncWebServer server(80);

auto keys = M5Cardputer.Keyboard.keysState();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    LOG(LOG_SYSTEM, "WebSocket client connected: " + String(client->id()));
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

// Forward declarations
void onLongPress();
void toggleWiFi();
void toggleWardriving();
void switchGPSSource();
void startWebLogServer();
void stopWebLogServer();

// Button long press tracking
unsigned long buttonPressStart = 0;
bool buttonHeld = false;
bool wifiStarted = false;

// GPS and wardriving
GPSManager gpsManager;
WigleLogger wigleLogger;

void setup() {
  M5.Power.begin();
  M5Cardputer.begin();
  Serial.begin(115200);
  delay(500);

  Screenshot::init();

  M5.Lcd.setSwapBytes(true);

  LOG(LOG_SYSTEM, "GhostBLE starting...");

  taskMutex = xSemaphoreCreateMutex();

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(0x00C4);
  delay(250);

  NimBLEDevice::init(DEVICE_NAME);
  registerGATTServiceHandlers();
  LOG(LOG_SYSTEM, "BLE initialized successfully.");

  // Start PwnBeacon advertising so other devices can discover us
  PwnBeaconServiceHandler::startAdvertising(DEVICE_NAME, DEVICE_FACE);

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  delay(200);

  // Deselect LoRa chip to free shared SPI bus for SD card
  #if defined(LORA_CS_PIN) && (LORA_CS_PIN >= 0)
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);
  #endif

  #if defined(CARDPUTER)
  if (!initLogger(SD_CS_PIN)) {
    drawThoughtBubble("NO SD CARD!", 125, 18);
    while (1);
  }
  #endif

  xpManager.begin();

  drawThoughtBubble("HI I'M NIBBLES", 125, 18);
  vTaskDelay(pdMS_TO_TICKS(2000));

  isWebLogActive = true;
  startWebLogServer();
  logEnableTarget(TARGET_WEB);

  ws.textAll("BLE_SCAN_READY");

  nibblesSpeechBegin();

  scanIsRunning = false;

  delay(3000);

  toggleWiFi();

  // To Update Wifi Logo to ON
  showFindingCounter(targetConnects, susDevice, leakedCounter);
}


void loop() {
  ws.cleanupClients();  // Important for websocket memory management
  M5Cardputer.update();
  Screenshot::handle();
  unsigned long currentTime = millis();

  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      auto status = M5Cardputer.Keyboard.keysState();

      if (status.enter) {
        LOG(LOG_CONTROL, "ENTER pressed");
        Screenshot::capture();
      }
      if (status.fn) {
        LOG(LOG_CONTROL, "FN pressed");
        toggleWiFi();
      }
      if (status.tab){
        LOG(LOG_CONTROL, "TAB pressed");
        toggleWardriving();
      }
      if (status.del){
        LOG(LOG_CONTROL, "DEL pressed");
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

  // Reactive memory cleanup: clear seenDevices when heap runs low or set grows too large,
  // rather than on a fixed timer. This avoids both premature clearing (losing dedup)
  // and late clearing (OOM risk).
  if (!seenDevices.empty() &&
      (seenDevices.size() >= MAX_SEEN_DEVICES || ESP.getFreeHeap() < MIN_FREE_HEAP_BYTES)) {
    LOG(LOG_SYSTEM, "Reactive cleanup (size: " + String(seenDevices.size()) +
                    ", free heap: " + String(ESP.getFreeHeap()) + ")");
    std::unordered_set<std::string>().swap(seenDevices);
    deviceSessionMap.clear();
    LOG(LOG_SYSTEM, "CLEAR SEEN DEVICES");
  }
  // Auto-enable serial logging when USB host is connected
  static bool lastUsbState = false;
  bool usbConnected = Serial;
  if (usbConnected != lastUsbState) {
    if (usbConnected) {
      logEnableTarget(TARGET_SERIAL);
    } else {
      logDisableTarget(TARGET_SERIAL);
    }
    lastUsbState = usbConnected;
  }

  // Let system handle BLE, GPIO, etc.
  yield();
}

void onLongPress() {
  bleScanEnabledWeb = !bleScanEnabledWeb;

  if (bleScanEnabledWeb) {
    LOG(LOG_CONTROL,"▶️ BLE Scan ENABLED");
    ws.textAll("BLE_SCAN_ON");
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                  nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
    delay(1500); 
    logNewBoot();   
    delay(500);             
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);
    nibblesSpeechShow(SpeechContext::SCAN_START);
  }
  else {
    LOG(LOG_CONTROL,"⏹️ BLE Scan DISABLED");
    ws.textAll("BLE_SCAN_OFF");
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                  nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);
    stopBleScan();   // THIS is the important part
  }
}

void toggleWiFi() {
  if (wifiStarted) {
    LOG(LOG_CONTROL,"WIFI / WEB SERVER OFF");
    stopWebLogServer();
    wifiStarted = false;
    isWebLogActive = false;
    logDisableTarget(TARGET_WEB);
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    }
    showFindingCounter(targetConnects, susDevice, leakedCounter); // optional: Icon ON
  } else {
    LOG(LOG_CONTROL,"WIFI / WEB SERVER ON");
    startWebLogServer();
    isWebLogActive = true;
    logEnableTarget(TARGET_WEB);
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    }
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

  LOG(LOG_CONTROL, "WiFi fully stopped");
}


void startWebLogServer() {
  LOG(LOG_CONTROL,"WEB SERVER");
  if (wifiStarted) return;  // Don't start twice
  WiFi.mode(WIFI_AP);
  LOG(LOG_CONTROL, "   - SoftAP started? YES");
  WiFi.softAP(ap_ssid, ap_password);
  LOG(LOG_CONTROL, "   - Access Point IP: " + WiFi.softAPIP().toString());

  // Setup WebSocket events and handlers BEFORE starting the server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
  LOG(LOG_CONTROL,"   - Web server started.");
  wifiStarted = true;

  LOG(LOG_CONTROL,"Pres BtnG0 to TOGGLE BLE Scan");
}

void toggleWardriving() {
  wardrivingEnabled = !wardrivingEnabled;

  if (wardrivingEnabled) {
    gpsManager.begin(GPSSource::GROVE);
    wigleLogger.begin();
    LOG(LOG_CONTROL,"Wardriving ON (" + String(gpsManager.getSourceName()) + ")");
    LOG(LOG_CONTROL,"  File: " + wigleLogger.getFilename());
  } else {
    wigleLogger.end();
    LOG(LOG_CONTROL,"Wardriving OFF (" + String(wigleLogger.getLoggedCount()) + " logged)");
  }

  ws.textAll(wardrivingEnabled ? "WARDRIVE_ON" : "WARDRIVE_OFF");
  drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);
  if (wardrivingEnabled) {
    nibblesSpeechShow(SpeechContext::WARDRIVING);
  }
}

void switchGPSSource() {
  if (!wardrivingEnabled) {
    LOG(LOG_CONTROL,"Enable wardriving first (TAB)");
    return;
  }

  GPSSource next = (gpsManager.getSource() == GPSSource::GROVE)
                   ? GPSSource::LORA_CAP
                   : GPSSource::GROVE;
  gpsManager.switchSource(next);
  LOG(LOG_CONTROL,"GPS: " + String(gpsManager.getSourceName()));

  drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);
}
