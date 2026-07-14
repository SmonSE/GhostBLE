
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

#include "ui/menu/menu_controller.h"


static MenuState menuState;  // globale Instanz
TaskHandle_t scanTaskHandle = NULL;


void scanTask(void* parameter) {
  while (true) {

    if (ScanContext::bleScanEnabled && !ScanContext::scanIsRunning) {
      nibblesSpeechNotifyEvent();
      scanForDevices();
    }
    vTaskDelay(pdMS_TO_TICKS(200)); // wichtig für Stabilität
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

void setup() {
  hardwareBegin();

  #if DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println("Debug active");
  #endif
  delay(500);

  #ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        LOG(LOG_SYSTEM, "PSRAM: " + String(ESP.getPsramSize() / 1024) + " KB");
    }
  #endif

  #if defined(CARDPUTER) 
    Screenshot::init();
  #endif

  M5.Lcd.setSwapBytes(true);
  LOG(LOG_SYSTEM, "GhostBLE starting...");

  UIContext::init();

  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(0x00C4);
  delay(250);

  DeviceContext::deviceConfig.begin();
  NimBLEDevice::init(DeviceContext::deviceConfig.getEffectiveBleName().c_str());

  registerGATTServiceHandlers();
  LOG(LOG_SYSTEM, "BLE initialized successfully.");

  // Start PwnBeacon advertising so other devices can discover us
  if (!DeviceContext::deviceConfig.getStealthMode()) {
      PwnBeaconServiceHandler::startAdvertising(
          DeviceContext::deviceConfig.getName(),
          DeviceContext::deviceConfig.getFace()
      );
  }
  
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  delay(200);

  // Deselect LoRa chip to free shared SPI bus for SD card
  #if defined(LORA_CS_PIN) && (LORA_CS_PIN >= 0)
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);
  #endif

#if HAS_SD_CARD
    #if defined(M5STICKS3)
        // Custom SPI pins for externally wired SD card
        SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    #endif
    if (!initLogger(SD_CS_PIN)) {
        drawThoughtBubble("NO SD CARD!", 125, 18);
        vTaskDelay(pdMS_TO_TICKS(3000));  // 3s anzeigen dann weitermachen
    }
#else
    initLogger(-1);
#endif

  MenuController::init(&menuState);
  menuSettings.begin();   // ← load stored values
  
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

  logEnableTarget(TARGET_WEB);

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
        if (MenuController::isOpen()) {
            LOG(LOG_CONTROL, "ENTER pressed — menu select");
            MenuController::selectCurrent();
        } else {
            LOG(LOG_CONTROL, "ENTER pressed");
            Screenshot::capture();
        }
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
        if (key == 'm' || key == 'M') {
          if (MenuController::isOpen()) {
              LOG(LOG_CONTROL, "M pressed — closing main menu");
              MenuController::close();
          } else {
              LOG(LOG_CONTROL, "M pressed — showing main menu");
              MenuController::open();
          }
          return;
        }
        if (key == 'i' || key == 'I') {
            LOG(LOG_CONTROL, "I pressed");
            Screenshot::capture();
          return;
        }
        if (key == 'h' || key == 'H') {
          if (UIContext::helpOverlayVisible) {
            LOG(LOG_CONTROL, "H pressed — closing help");
            UIContext::hideHelpOverlay();
          } else {
            LOG(LOG_CONTROL, "H pressed — showing help");
            showHelpOverlay();
          }
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
        if ((key == 'r' || key == 'R')) {
          LOG(LOG_CONTROL, "R pressed — toggling research mode");
          if(!UIContext::isResearchModeActive) {
            UIContext::isResearchModeActive = true;
            showResearchMode();
          } else {
            UIContext::isResearchModeActive = false;
            showResearchMode();
          }
          return;
        }
        if ((key == 'p' || key == 'P') && ScanContext::bleScanEnabled) {
            LOG(LOG_CONTROL, "P pressed — pointer set");

            bool hasFix = NetworkContext::gpsManager.isValid();

            DeviceContext::pointer++;

            char msg[160];

            if (NetworkContext::wardrivingEnabled.load() && hasFix) {
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
      if (MenuController::isOpen()) {
        for (auto key : status.word) {
            if (key == ';') {
                MenuController::navigateUp();
            }
            if (key == '.') {
                MenuController::navigateDown();
            }
            if (key == ',') {
                MenuController::adjustLeft();
            }
            if (key == '/') {
                MenuController::adjustRight();
            }
        }
        return;
      }
    }
  }
#endif

// ================================================================
//  Cardputer Button Handling
// Button A: long press (1s) = toggle BLE scan
//           short press = toggle WiFi (on 2-button devices)
//           3s hold = help overlay (on 2-button devices)
// ================================================================ 
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
// ================================================================
//  StickS3 Button Handling
//    BtnA short = navigate down (menu open only)
//    BtnA 3s    = BLE Scan toggle
//    BtnB short = select / toggle / increase slider (menu open only)
//    BtnB 3s    = open/close Main Menu
// ================================================================
#else
  // ── Help overlay dismiss (any button) ──────────────────────
  if (UIContext::helpOverlayVisible) {
    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {
      LOG(LOG_CONTROL, "Button pressed — closing help");
      UIContext::hideHelpOverlay();
    }
    return;
  }
  // ── Button A ─────────────────────────────────────────────────
  if (M5.BtnA.isPressed()) {
    if (!buttonAHeld) {
      if (buttonAPressStart == 0) {
        buttonAPressStart = currentTime;
      } else if (currentTime - buttonAPressStart >= HELP_LONG_PRESS_MS) {
        buttonAHeld = true;
        LOG(LOG_CONTROL, "BtnA 3s — BLE scan toggle");
        onLongPress();
      }
    }
  } else {
    if (buttonAPressStart > 0 && !buttonAHeld) {
      unsigned long held = currentTime - buttonAPressStart;
      if (held < LONG_PRESS_MS) {
        if (MenuController::isOpen()) {
          LOG(LOG_CONTROL, "BtnA short — menu navigate down");
          MenuController::navigateDown();
        }
      }
    }
    buttonAPressStart = 0;
    buttonAHeld       = false;
  }

  // ── Button B ─────────────────────────────────────────────────
  #if HAS_TWO_BUTTONS
  if (M5.BtnB.isPressed()) {
    if (!buttonBHeld) {
      if (buttonBPressStart == 0) {
        buttonBPressStart = currentTime;
      } else if (currentTime - buttonBPressStart >= HELP_LONG_PRESS_MS) {
        buttonBHeld = true;
        if (MenuController::isOpen()) {
          LOG(LOG_CONTROL, "BtnB 3s — closing menu");
          MenuController::close();
        } else {
          LOG(LOG_CONTROL, "BtnB 3s — opening menu");
          MenuController::open();
        }
      }
    }
  } else {
    if (buttonBPressStart > 0 && !buttonBHeld) {
      unsigned long held = currentTime - buttonBPressStart;
      if (held < LONG_PRESS_MS) {
        if (MenuController::isOpen()) {
          LOG(LOG_CONTROL, "BtnB short — menu select/toggle");
          MenuController::selectCurrent();
        }
      }
    }
    buttonBPressStart = 0;
    buttonBHeld       = false;
    buttonBShortHandled = false;
  }
  #endif  // HAS_TWO_BUTTONS
#endif // !CARDPUTER

  // Update GPS if wardriving is active
  if (NetworkContext::wardrivingEnabled.load()) {
    NetworkContext::gpsManager.update();

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
    LOG(LOG_CONTROL,"▶️ BLE Scan ENABLED");
    if(!MenuController::isOpen()) {
          drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                  nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
    }
    
    delay(1000);
    logNewBoot();
    delay(500);
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
  }
  else {
    LOG(LOG_CONTROL,"⏹️ BLE Scan DISABLED");
    if(!MenuController::isOpen()) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                    nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    }
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
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
    if(!MenuController::isOpen()) {
      if (random(2) == 0) {
        drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                      nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
      } else {
        drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                      nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
      }
    }
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice); // optional: Icon ON
  } else {
    LOG(LOG_CONTROL,"WIFI / WEB SERVER ON");
    startWebLogServer();
    NetworkContext::wifiStarted = true;
    NetworkContext::isWebLogActive = true;
    logEnableTarget(TARGET_WEB);
    if(!MenuController::isOpen()) {
      if (random(2) == 0) {
        drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                      nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
      } else {
        drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                      nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
      }
    }
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice); // optional: Icon ON
  }

}

void toggleWardriving() {
  Serial.printf("call toggleWardriving\n");
    if (NetworkContext::wardrivingEnabled.load()) {
        Serial.printf("Toggle Wardrive enabled\n");
        UIContext::isResearchModeActive = false; // disable research mode when wardriving off
        NetworkContext::wardrivingEnabled.store(false);
        delay(100); // ensure any ongoing logging finishes before stopping GPS and file
        NetworkContext::wigleLogger.end();
        LOG(LOG_CONTROL, "Wardriving OFF (" +
        String(NetworkContext::wigleLogger.getLoggedCount()) + " logged)");
        logDisableCategory(LOG_GPS);
        
        if(!MenuController::isOpen()) {
          if (random(2) == 0) {
            drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                          nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
          } else {
            drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                          nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
          }
        }
        showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
    } else {
        Serial.printf("Toggle Wardrive enabled\n");
        UIContext::isResearchModeActive = true; // enable research mode for wardriving to get aggressive setup
        NetworkContext::gpsManager.begin(GPSSource::GROVE);
        NetworkContext::wigleLogger.begin();
        delay(100); // ensure wigle logger is ready before enabling wardriving
        LOG(LOG_CONTROL, "Wardriving ON  (" + String(NetworkContext::gpsManager.getSourceName()) + ")");
        LOG(LOG_CONTROL, "  File: " + NetworkContext::wigleLogger.getFilename());

        if(!MenuController::isOpen()) {
          if (random(2) == 0) {
            drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                          nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, 83, 60);
          } else {
            drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                          nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
          }
        }
        showResearchMode();
        showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
    }
}

void switchGPSSource()  { NetworkContext::switchGPSSource();  }
void startWebLogServer(){ NetworkContext::startWebServer();   }
void stopWebLogServer() { NetworkContext::stopWebServer();    }
