#include "src/config/hardware.h"
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
#include "src/config/DeviceConfig.h"

#include <WiFi.h>
#include <AsyncTCP.h>


// Reactive memory cleanup uses heap threshold + set size (see loop)


AsyncWebServer server(80);

#if HAS_KEYBOARD
auto keys = M5Cardputer.Keyboard.keysState();
#endif

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    LOG(LOG_SYSTEM, "WebSocket client connected: " + String(client->id()));
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = '\0';
      String msg = String((char*)data);
      String reply = deviceConfig.handleMessage(msg);
      if (reply.length() > 0) {
        client->text(reply);
      }
    }
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
unsigned long buttonAPressStart = 0;
bool buttonAHeld = false;
bool wifiStarted = false;

#if HAS_TWO_BUTTONS
unsigned long buttonBPressStart = 0;
bool buttonBHeld = false;
bool buttonBShortHandled = false;
#endif

// GPS and wardriving
GPSManager gpsManager;
WigleLogger wigleLogger;

void setup() {
  hardwareBegin();
  Serial.begin(115200);
  delay(500);

  Screenshot::init();

  M5.Lcd.setSwapBytes(true);

  LOG(LOG_SYSTEM, "GhostBLE starting...");

  taskMutex = xSemaphoreCreateMutex();

  M5.Lcd.setRotation(1);

  M5.Lcd.fillScreen(0x00C4);
  delay(250);

  deviceConfig.begin();

  NimBLEDevice::init(deviceConfig.getName().c_str());
  registerGATTServiceHandlers();
  LOG(LOG_SYSTEM, "BLE initialized successfully.");

  // Start PwnBeacon advertising so other devices can discover us
  PwnBeaconServiceHandler::startAdvertising(deviceConfig.getName(), deviceConfig.getFace());

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  delay(200);

  // Deselect LoRa chip to free shared SPI bus for SD card
  #if defined(LORA_CS_PIN) && (LORA_CS_PIN >= 0)
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);
  #endif

  #if HAS_SD_CARD
  if (!initLogger(SD_CS_PIN)) {
    drawThoughtBubble("NO SD CARD!", 125, 18);
    while (1);
  }
  #else
  // No SD card on this device — initialize logger without SD
  initLogger(-1);
  #endif

  xpManager.begin();

  drawThoughtBubble("HI I'M NIBBLES", BUBBLE_X, THOUGHT_BUBBLE_Y);
  vTaskDelay(pdMS_TO_TICKS(2000));

  clearSpeechBubble();
#if HAS_KEYBOARD
  drawThoughtBubble("PRESS H FOR HELP!", BUBBLE_X, THOUGHT_BUBBLE_Y);
#else
  drawThoughtBubble("HOLD M5 FOR HELP!", BUBBLE_X, THOUGHT_BUBBLE_Y);
#endif
  vTaskDelay(pdMS_TO_TICKS(3000));
  clearSpeechBubble();

  isWebLogActive = true;
  startWebLogServer();
  logEnableTarget(TARGET_WEB);

  ws.textAll("BLE_SCAN_READY");

  nibblesSpeechBegin();

  scanIsRunning = false;

  delay(1000);

  toggleWiFi();

  // To Update Wifi Logo to ON
  showFindingCounter(targetConnects, susDevice, leakedCounter);
}


void loop() {
  ws.cleanupClients();  // Important for websocket memory management
  hardwareUpdate();
  unsigned long currentTime = millis();

  // ===== Input Handling =====

  // When help overlay is visible, any input dismisses it
  if (helpOverlayVisible) {
#if HAS_KEYBOARD
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      dismissHelpOverlay();
      return;
    }
#endif
#if defined(CARDPUTER)
    if (M5Cardputer.BtnA.wasPressed()) { dismissHelpOverlay(); return; }
#else
    if (M5.BtnA.wasPressed()) { dismissHelpOverlay(); return; }
#if HAS_TWO_BUTTONS
    if (M5.BtnB.wasPressed()) { dismissHelpOverlay(); return; }
#endif
#endif
    return;  // skip all other processing while help is visible
  }

#if HAS_KEYBOARD
  // Cardputer keyboard input
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
      // 'h' key opens help overlay
      for (auto key : status.word) {
        if (key == 'h' || key == 'H') {
          LOG(LOG_CONTROL, "H pressed — showing help");
          showHelpOverlay();
          return;
        }
      }
    }
  }
#endif

  // Button A: long press (1s) = toggle BLE scan
  //           short press = toggle WiFi (on 2-button devices)
  //           3s hold = help overlay (on 2-button devices)
#if defined(CARDPUTER)
  if (M5Cardputer.BtnA.isPressed()) {
    if (!buttonAHeld) {
      if (buttonAPressStart == 0) {
        buttonAPressStart = currentTime;
      } else if (currentTime - buttonAPressStart >= LONG_PRESS_MS) {
        buttonAHeld = true;
        onLongPress();
      }
    }
  } else {
    buttonAPressStart = 0;
    buttonAHeld = false;
  }
#else
  if (M5.BtnA.isPressed()) {
    if (!buttonAHeld) {
      if (buttonAPressStart == 0) {
        buttonAPressStart = currentTime;
      } else if (currentTime - buttonAPressStart >= HELP_LONG_PRESS_MS) {
        // 3s hold: show help overlay
        buttonAHeld = true;
        LOG(LOG_CONTROL, "BtnA 3s hold — showing help");
        showHelpOverlay();
      }
      // Note: 1s BLE toggle deferred to release to distinguish from 3s help
    }
  } else {
    if (buttonAPressStart > 0 && !buttonAHeld) {
      unsigned long held = currentTime - buttonAPressStart;
      if (held >= LONG_PRESS_MS) {
        // Released between 1-3s: toggle BLE scan
        LOG(LOG_CONTROL, "BtnA long press");
        onLongPress();
      } else {
        // Short press: toggle WiFi
        LOG(LOG_CONTROL, "BtnA short press");
        toggleWiFi();
      }
    }
    buttonAPressStart = 0;
    buttonAHeld = false;
  }
#endif

#if HAS_TWO_BUTTONS
  // Button B: long press = switch GPS source
  //           short press = toggle wardriving
  if (M5.BtnB.isPressed()) {
    if (!buttonBHeld) {
      if (buttonBPressStart == 0) {
        buttonBPressStart = currentTime;
      } else if (currentTime - buttonBPressStart >= LONG_PRESS_MS) {
        buttonBHeld = true;
        buttonBShortHandled = true;
        LOG(LOG_CONTROL, "BtnB long press");
        switchGPSSource();
      }
    }
  } else {
    // Short press detection: was pressed but not held long enough
    if (buttonBPressStart > 0 && !buttonBHeld) {
      LOG(LOG_CONTROL, "BtnB short press");
      toggleWardriving();
    }
    buttonBPressStart = 0;
    buttonBHeld = false;
    buttonBShortHandled = false;
  }
#endif

  // Update GPS if wardriving is active
  if (wardrivingEnabled) {
    gpsManager.update();

    // Refresh GPS status bar every second
    static unsigned long lastGPSDisplayUpdate = 0;
    if (currentTime - lastGPSDisplayUpdate >= 1000) {
      lastGPSDisplayUpdate = currentTime;
      showFindingCounter(targetConnects, susDevice, allSpottedDevice);
    }
  }

  // NibBLEs speech system (idle mumbling)
  nibblesSpeechUpdate(currentTime);

  // BLE scan loop (non-blocking: runs in FreeRTOS task on core 0)
  if (bleScanEnabled) {
    if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
      if (!targetFound && !scanIsRunning && scanTaskHandle == NULL) {
        nibblesSpeechNotifyEvent();
        xTaskCreatePinnedToCore(scanForDevicesTask, "BLEScan", 8192, NULL, 1, &scanTaskHandle, 0);
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
  bleScanEnabled = !bleScanEnabled;

  if (bleScanEnabled) {
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
  WiFi.softAP(deviceConfig.getWifiSSID().c_str(), deviceConfig.getWifiPassword().c_str());
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

  LOG(LOG_CONTROL,"Press BtnA (long) to TOGGLE BLE Scan");
}

void toggleWardriving() {
  wardrivingEnabled = !wardrivingEnabled;

  if (wardrivingEnabled) {
    if (!bleScanEnabled) {
      bleScanEnabled = true;
      LOG(LOG_CONTROL,"▶️ BLE Scan ENABLED (wardriving)");
      ws.textAll("BLE_SCAN_ON");
    }
    logEnableCategory(LOG_GPS);
    gpsManager.begin(GPSSource::GROVE);
    wigleLogger.begin();
    LOG(LOG_CONTROL,"Wardriving ON (" + String(gpsManager.getSourceName()) + ")");
    LOG(LOG_CONTROL,"  File: " + wigleLogger.getFilename());
  } else {
    wigleLogger.end();
    LOG(LOG_CONTROL,"Wardriving OFF (" + String(wigleLogger.getLoggedCount()) + " logged)");
    logDisableCategory(LOG_GPS);
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
    LOG(LOG_CONTROL,"Enable wardriving first");
    return;
  }

#if defined(LORA_CS_PIN)
  GPSSource next = (gpsManager.getSource() == GPSSource::GROVE)
                   ? GPSSource::LORA_CAP
                   : GPSSource::GROVE;
  gpsManager.switchSource(next);
  LOG(LOG_CONTROL,"GPS: " + String(gpsManager.getSourceName()));
#else
  LOG(LOG_CONTROL,"Only Grove GPS available on this device");
#endif

  drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);
}
