
#include <SPI.h>
#include <vector>
#include <unordered_set>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <AsyncTCP.h>

#include "app/context/ui_context.h"
#include "app/context/network_context.h"
#include "app/context/device_context.h"
#include "app/interaction/nibbles_speech.h"

#include "src/assets/nibblesFront.h"
#include "src/assets/nibblesGlasses.h"
#include "src/assets/nibblesAngry.h"
#include "src/assets/nibblesSad.h"
#include "src/assets/nibblesHappy.h"
#include "src/assets/nibblesHappyLeft.h"
#include "src/assets/nibblesThugLife.h"

#include "src/config/ui_config.h"
#include "src/config/app_config.h"
#include "src/config/device_config.h"
#include "src/config/scan_config.h"

#include "src/infrastructure/ble/ble_scanner.h"
#include "src/infrastructure/ble/gattServices/init_gatt_service.h"
#include "src/infrastructure/ble/gattServices/pwn_beacon_service.h"
#include "src/infrastructure/gps/gps_manager.h"
#include "src/infrastructure/logging/logger.h"
#include "src/infrastructure/platform/hardware.h"
#include "src/infrastructure/platform/hardware_config.h"
#include "src/infrastructure/storage/screenshot.h"
#include "src/infrastructure/wardriving/wigle_logger.h"

#include "src/ui/icons/scan_icon.h"
#include "src/ui/overlay/draw_overlay.h"
#include "src/ui/expression/show_expression.h"

TaskHandle_t scanTaskHandle = NULL;

#if HAS_KEYBOARD
auto keys = M5Cardputer.Keyboard.keysState();
#endif

void scanTask(void* parameter) {
  while (true) {

    if (ScanContext::bleScanEnabled && !ScanContext::scanIsRunning) {
      nibblesSpeechNotifyEvent();
      scanForDevices();
    }

    vTaskDelay(pdMS_TO_TICKS(200)); // wichtig für Stabilität
  }
}

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

#if defined(CARDPUTER) 
  Screenshot::init();
#endif

  M5.Lcd.setSwapBytes(true);

  LOG(LOG_SYSTEM, "GhostBLE starting...");

  //taskMutex = xSemaphoreCreateMutex();
  UIContext::init();

  M5.Lcd.setRotation(1);

  M5.Lcd.fillScreen(0x00C4);
  delay(250);

  DeviceContext::deviceConfig.begin();

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

  DeviceContext::xpManager.begin();

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

  showScanIcon();

  NetworkContext::isWebLogActive = false;
  logEnableTarget(TARGET_WEB);

  ws.textAll("BLE_SCAN_READY");

  nibblesSpeechBegin();


  ScanContext::scanIsRunning = false;

  delay(500);

  // To Update Wifi Logo to ON
  showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::leakedCounter);

  // Start Scan Task (FreeRTOS)
  xTaskCreatePinnedToCore(scanTask, "ScanTask", 12000, NULL, 1, &scanTaskHandle, 1);
}


void loop() {
#if defined(CARDPUTER)  
  Screenshot::handle();
#endif  
  
  static unsigned long lastCleanup = 0;
  if (NetworkContext::isWebLogActive && millis() - lastCleanup > 1000) {
      ws.cleanupClients();
      lastCleanup = millis();
  }

  hardwareUpdate();
  unsigned long currentTime = millis();

  // ===== Input Handling =====

  // When help overlay is visible, any input dismisses it
  if (UIContext::helpOverlayVisible) {
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
      if (status.fn && !ScanContext::bleScanEnabled) {
        LOG(LOG_CONTROL, "FN pressed");
        toggleWiFi();
      }
      if (status.tab && !ScanContext::bleScanEnabled){
        LOG(LOG_CONTROL, "TAB pressed");
        printf("Toggle Wardrive\n");
        toggleWardriving();
      }
      if (status.del && !ScanContext::bleScanEnabled){
        LOG(LOG_CONTROL, "DEL pressed");
        printf("switchGPSSource\n");
        NetworkContext::switchGPSSource();
      }
      // 'h' key opens help overlay
      for (auto key : status.word) {
        if (key == 'h' || key == 'H') {
          LOG(LOG_CONTROL, "H pressed — showing help");
          showHelpOverlay();
          return;
        }
        // Scan Mode Toggle
        if ((key == 's' || key == 'S')) {
          LOG(LOG_CONTROL, "S pressed — toggling scan mode");
          if(!ScanContext::bleScanEnabled) {
            toggleScanMode();
          }
          return;
        }
        if ((key == 'm' || key == 'M') && ScanContext::bleScanEnabled) {
            LOG(LOG_CONTROL, "M pressed — marker set");

            bool hasFix = NetworkContext::gpsManager.isValid();

            DeviceContext::pointer++;

            char msg[160];

            if (NetworkContext::wardrivingEnabled && hasFix) {
                snprintf(msg, sizeof(msg),
                        "[MARKER #%d][GPS] Time:%s SAT:%u Lat: %.6f Lon: %.6f",
                        DeviceContext::pointer.load(),
                        NetworkContext::gpsManager.getTimestamp().c_str(),
                        NetworkContext::gpsManager.getSatellites(),
                        NetworkContext::gpsManager.getLatitude(),
                        NetworkContext::gpsManager.getLongitude());
            } else {
                snprintf(msg, sizeof(msg),
                        "[MARKER #%d][NO GPS]",
                        DeviceContext::pointer.load());
            }

            drawPointer(DeviceContext::pointer.load());  // Update on-screen pointer count immediately

            LOG(LOG_GATT, msg);
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
        NetworkContext::switchGPSSource();
      }
    }
  } else {
    // Short press detection: was pressed but not held long enough
    if (buttonBPressStart > 0 && !buttonBHeld) {
      LOG(LOG_CONTROL, "BtnB short press");
      NetworkContext::toggleWardriving();
    }
    buttonBPressStart = 0;
    buttonBHeld = false;
    buttonBShortHandled = false;
  }
#endif

  // Update GPS if wardriving is active
  if (NetworkContext::wardrivingEnabled) {
    gpsManager.update();

    // Refresh GPS status bar every second
    static unsigned long lastGPSDisplayUpdate = 0;
    if (currentTime - lastGPSDisplayUpdate >= 1000) {
      lastGPSDisplayUpdate = currentTime;
      showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
    }
  }

  // NibBLEs speech system (idle mumbling)
  nibblesSpeechUpdate(currentTime);

  // Reactive memory cleanup: clear seenDevices when heap runs low or set grows too large,
  // rather than on a fixed timer. This avoids both premature clearing (losing dedup)
  // and late clearing (OOM risk).
  if (!ScanContext::seenDevices.empty() &&
      (ScanContext::seenDevices.size() >= MAX_SEEN_DEVICES || ESP.getFreeHeap() < MIN_FREE_HEAP_BYTES)) {
    LOG(LOG_SYSTEM, "Reactive cleanup (size: " + String(ScanContext::seenDevices.size()) +
                    ", free heap: " + String(ESP.getFreeHeap()) + ")");
    std::unordered_set<std::string>().swap(ScanContext::seenDevices);
    ScanContext::deviceSessionMap.clear();
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
  ScanContext::bleScanEnabled = !ScanContext::bleScanEnabled;

  if (ScanContext::bleScanEnabled) {
    //nibblesSpeechShow(SpeechContext::SCAN_START); //to much for queue!
    //delay(1000);
    LOG(LOG_CONTROL,"▶️ BLE Scan ENABLED");
    ws.textAll("BLE_SCAN_ON");
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                  nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
    delay(1000);
    logNewBoot();
    delay(500);
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
  }
  else {
    LOG(LOG_CONTROL,"⏹️ BLE Scan DISABLED");
    ws.textAll("BLE_SCAN_OFF");
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                  nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
    //nibblesSpeechShow(SpeechContext::SCAN_STOP); //to much for queue!
    stopBleScan();   // THIS is the important part
  }
}

void toggleWiFi() {
  if (NetworkContext::wifiStarted) {
    LOG(LOG_CONTROL,"WIFI / WEB SERVER OFF");
    stopWebLogServer();
    NetworkContext::wifiStarted = false;
    NetworkContext::isWebLogActive = false;
    logDisableTarget(TARGET_WEB);
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    }
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::leakedCounter); // optional: Icon ON
  } else {
    LOG(LOG_CONTROL,"WIFI / WEB SERVER ON");
    startWebLogServer();
    NetworkContext::isWebLogActive = true;
    logEnableTarget(TARGET_WEB);
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    }
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::leakedCounter); // optional: Icon ON
  }

}

void toggleWardriving() {
  printf("call toggleWardriving\n");
    if (NetworkContext::wardrivingEnabled) {
        printf("Toggle Wardrive enabled (if)\n");
        NetworkContext::wardrivingEnabled = false;
        wigleLogger.end();
        LOG(LOG_CONTROL, "Wardriving OFF (" +
        String(wigleLogger.getLoggedCount()) + " logged)");
        logDisableCategory(LOG_GPS);
        
        if (random(2) == 0) {
          drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                        nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
        } else {
          drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                        nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
        }
        showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::leakedCounter);
    } else {
        printf("Toggle Wardrive enabled (else)\n");
        logEnableCategory(LOG_GPS);
        gpsManager.begin(GPSSource::GROVE);
        wigleLogger.begin();
        LOG(LOG_CONTROL, "Wardriving ON  (" + String(gpsManager.getSourceName()) + ")");
        LOG(LOG_CONTROL, "  File: " + wigleLogger.getFilename());

        if (random(2) == 0) {
          drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
        } else {
          drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                        nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
        }
        showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::leakedCounter);
    }
    ws.textAll(NetworkContext::wardrivingEnabled ? "WARDRIVE_ON" : "WARDRIVE_OFF");
}

void switchGPSSource()  { NetworkContext::switchGPSSource();  }
void startWebLogServer(){ NetworkContext::startWebServer();   }
void stopWebLogServer() { NetworkContext::stopWebServer();    }
