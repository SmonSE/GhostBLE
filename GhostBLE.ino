
#include <SPI.h>
#include <vector>
#include <unordered_set>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <AsyncTCP.h>

#include "app/context/ui_context.h"
#include "app/context/network_context.h"
#include "app/context/device_context.h"
#include "app/context/device_finder.h"
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
#include "src/ui/susview/sus_device_view.h"
#include "src/ui/finder/finder_list_view.h"
#include "src/ui/finder/approach_view.h"

#include "ui/menu/menu_controller.h"
#include "ui/filemanager/file_manager_view.h"


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
bool waitForBtnRelease = false;

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
  NimBLEDevice::setMTU(247);

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

if (!DeviceContext::deviceConfig.getFirstBootDone()) {
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

    DeviceContext::deviceConfig.setFirstBootDone(true);
} else {
    nibblesSpeechShow(SpeechContext::WELCOME_BACK);
    vTaskDelay(pdMS_TO_TICKS(2000));
    clearSpeechBubble();
}

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

  // ── Approach View — periodischer Scan + Redraw, unabhängig von Tasteneingaben ──
  if (ApproachView::isOpen()) {
    ApproachView::update();
  }

  // ===== Input Handling =====
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
    return;
  }

#if HAS_KEYBOARD
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      auto status = M5Cardputer.Keyboard.keysState();

      // ── ENTER: confirm/select — routed to whichever view is active ──
      if (status.enter) {
          if (FinderListView::isOpen()) {
              LOG(LOG_CONTROL, "ENTER — finder select");
              FinderListView::selectCurrent();
          } else if (SusDeviceView::isOpen()) {
              LOG(LOG_CONTROL, "ENTER — sus device select");
              SusDeviceView::selectCurrent();
          } else if (FileManagerView::isInConfirmMode()) {
              LOG(LOG_CONTROL, "ENTER — confirming file delete");
              FileManagerView::confirmDelete();
          } else if (FileManagerView::isOpen()) {
              LOG(LOG_CONTROL, "ENTER — file manager select");
              FileManagerView::selectCurrent();
          } else if (MenuController::isOpen()) {
              LOG(LOG_CONTROL, "ENTER — menu select");
              MenuController::selectCurrent();
          }
          return;
      }

      // ── ESC: one step back — closes innermost open view first ──
      for (auto key : status.word) {
          if (key == '`') {
              if (ApproachView::isOpen()) {
                  ApproachView::close();
              } else if (FinderListView::isOpen()) {
                  FinderListView::close();
              } else if (SusDeviceView::isOpen()) {
                  SusDeviceView::close();
              } else if (FileManagerView::isInConfirmMode()) {
                  FileManagerView::cancelAction();
              } else if (FileManagerView::isOpen()) {
                  FileManagerView::close();
              } else if (MenuController::isOpen()) {
                  MenuController::close();
              }
              return;
          }
      }

      // ── Approach View: nur ESC wirkt, sonst nichts weiter verarbeiten ──
      if (ApproachView::isOpen()) {
        return;
      }

      // ── Finder List View: uniforme Navigation + eigene Refresh-Taste ──
      if (FinderListView::isOpen()) {
        for (auto key : status.word) {
          if (key == ';') FinderListView::navigatePrev();
          if (key == '.') FinderListView::navigateNext();
          if (key == 'f' || key == 'F') FinderListView::refresh();
        }
        return;
      }

      // ── Sus Device View: uniforme Navigation ──
      if (SusDeviceView::isOpen()) {
        for (auto key : status.word) {
          if (key == ';') SusDeviceView::navigatePrev();
          if (key == '.') SusDeviceView::navigateNext();
          if (key == '`') SusDeviceView::close();
        }
        return;
      }

      if (FileManagerView::isOpen()) {
          for (auto key : status.word) {
              if (key == ';') FileManagerView::navigatePrev();
              if (key == '.') FileManagerView::navigateNext();
          }
          if (status.enter) {
              FileManagerView::selectCurrent();
          }
          return;
      }

      // ── Main Menu: uniforme Navigation + Slider-Adjust ──
      if (MenuController::isOpen()) {
        for (auto key : status.word) {
          if (key == ';') MenuController::navigateUp();
          if (key == '.') MenuController::navigateDown();
          if (key == ',') MenuController::adjustLeft();
          if (key == '/') MenuController::adjustRight();
        }
        return;
      }

      // ── Nichts offen: globale Shortcuts ──
      if (status.fn && !ScanContext::bleScanEnabled) {
        LOG(LOG_CONTROL, "FN pressed");
        toggleWiFi();
      }
      if (status.tab && !ScanContext::bleScanEnabled) {
        LOG(LOG_CONTROL, "TAB pressed");
        toggleWardriving();
      }
      if (status.del && !ScanContext::bleScanEnabled) {
        LOG(LOG_CONTROL, "DEL pressed");
        NetworkContext::switchGPSSource();
      }

      for (auto key : status.word) {
        if (key == 'm' || key == 'M') {
          LOG(LOG_CONTROL, "M pressed — showing main menu");
          MenuController::open();
          return;
        }
        if (key == 'd' || key == 'D') {
            NetworkContext::displayEnabled = !NetworkContext::displayEnabled;

            if (NetworkContext::displayEnabled) {
                M5.Lcd.wakeup();
                LOG(LOG_CONTROL, "D pressed — display ON");
            } else {
                M5.Lcd.sleep();
                LOG(LOG_CONTROL, "D pressed — display OFF");
            }

            return;
        }
        if (key == 'f' || key == 'F') {
          LOG(LOG_CONTROL, "F pressed — Find Device");
          FinderListView::drawScanning();
          DeviceFinder::startFinderFlow();
          return;
        }
        if (key == 'i' || key == 'I') {
          LOG(LOG_CONTROL, "I pressed");
          Screenshot::capture();
          return;
        }
        if (key == 'h' || key == 'H') {
          LOG(LOG_CONTROL, "H pressed — showing help");
          showHelpOverlay();
          return;
        }
        if (key == 's' || key == 'S') {
          LOG(LOG_CONTROL, "S pressed — toggling scan mode");
          if (!ScanContext::bleScanEnabled) {
            toggleScanMode();
          }
          return;
        }
        if (key == 'r' || key == 'R') {
          LOG(LOG_CONTROL, "R pressed — toggling research mode");
          UIContext::isResearchModeActive = !UIContext::isResearchModeActive;
          showResearchMode();
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
            snprintf(msg, sizeof(msg), "[MARKER #%d][NO GPS]", DeviceContext::pointer.load());
          }

          drawPointer(DeviceContext::pointer.load());
          LOG(LOG_GATT, msg);
          return;
        }
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
#else
  // ================================================================
  //  StickS3 Button Handling
  //
  //  BtnA short = navigate next
  //  BtnA 3s    = BLE scan / refresh
  //  BtnB short = select / confirm / toggle
  //  BtnB 3s    = back one level / open menu
  //
  //  IMPORTANT:
  //  After a long-press action changes the current view, we wait
  //  until the physical button has been released before processing
  //  any new button input.
  // ================================================================

  // ────────────────────────────────────────────────────────────────
  // Wait for physical button release after a long-press action
  // ────────────────────────────────────────────────────────────────
  if (waitForBtnRelease) {

    if (!M5.BtnA.isPressed() && !M5.BtnB.isPressed()) {

      waitForBtnRelease = false;

      buttonAPressStart = 0;
      buttonBPressStart = 0;

      buttonAHeld = false;
      buttonBHeld = false;

      buttonBShortHandled = false;

      LOG(LOG_CONTROL, "Buttons released — input unlocked");
    }

    // Do NOT process any other button input while waiting.
    return;
  }


  // ────────────────────────────────────────────────────────────────
  // Help overlay dismiss (any button)
  // ────────────────────────────────────────────────────────────────
  if (UIContext::helpOverlayVisible) {

    if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) {

      LOG(LOG_CONTROL, "Button pressed — closing help");

      UIContext::hideHelpOverlay();

      // The button press that closed the overlay must not
      // immediately trigger another action.
      waitForBtnRelease = true;
    }

    return;
  }


  // ────────────────────────────────────────────────────────────────
  // Approach View — only BtnB(hold) closes
  // ────────────────────────────────────────────────────────────────
  if (ApproachView::isOpen()) {

    if (M5.BtnB.isPressed()) {

      if (buttonBPressStart == 0) {
        buttonBPressStart = currentTime;
      }
      else if (currentTime - buttonBPressStart >= HELP_LONG_PRESS_MS) {

        LOG(LOG_CONTROL, "BtnB 3s — closing Approach View");

        ApproachView::close();

        buttonBPressStart = 0;
        buttonBHeld = true;

        // IMPORTANT:
        // Do not allow this same physical press to be
        // interpreted by the next view.
        waitForBtnRelease = true;
      }

    } else {

      buttonBPressStart = 0;
    }

    return;
  }


  // ────────────────────────────────────────────────────────────────
  // Finder List View
  // ────────────────────────────────────────────────────────────────
  if (FinderListView::isOpen()) {

    // ── BtnA short = next, BtnA 3s = refresh ────────────────
    if (M5.BtnA.isPressed()) {

      if (!buttonAHeld) {

        if (buttonAPressStart == 0) {
          buttonAPressStart = currentTime;
        }
        else if (currentTime - buttonAPressStart >= HELP_LONG_PRESS_MS) {

          buttonAHeld = true;

          LOG(LOG_CONTROL, "BtnA 3s — refreshing finder");

          FinderListView::refresh();
        }
      }

    } else {

      if (buttonAPressStart > 0 && !buttonAHeld) {

        unsigned long held = currentTime - buttonAPressStart;

        if (held < LONG_PRESS_MS) {

          LOG(LOG_CONTROL, "BtnA short — finder next");

          FinderListView::navigateNext();
        }
      }

      buttonAPressStart = 0;
      buttonAHeld = false;
    }


    // ── BtnB short = select, BtnB 3s = close ────────────────
    if (M5.BtnB.isPressed()) {

      if (!buttonBHeld) {

        if (buttonBPressStart == 0) {
          buttonBPressStart = currentTime;
        }
        else if (currentTime - buttonBPressStart >= HELP_LONG_PRESS_MS) {

          buttonBHeld = true;

          LOG(LOG_CONTROL, "BtnB 3s — closing finder list");

          FinderListView::close();

          buttonBPressStart = 0;

          // Prevent the same long press from
          // entering/selecting something in the next view.
          waitForBtnRelease = true;
        }
      }

    } else {

      if (buttonBPressStart > 0 && !buttonBHeld) {

        unsigned long held = currentTime - buttonBPressStart;

        if (held < LONG_PRESS_MS) {

          LOG(LOG_CONTROL, "BtnB short — finder select");

          FinderListView::selectCurrent();
        }
      }

      buttonBPressStart = 0;
      buttonBHeld = false;
    }

    return;
  }


  // ────────────────────────────────────────────────────────────────
  // Sus Device View
  // ────────────────────────────────────────────────────────────────
  if (SusDeviceView::isOpen()) {

    // ── BtnA short = next ───────────────────────────────────
    if (M5.BtnA.wasPressed()) {

      SusDeviceView::navigateNext();
    }


    // ── BtnB short = locate, BtnB 3s = back ────────────────
    if (M5.BtnB.isPressed()) {

      if (!buttonBHeld) {

        if (buttonBPressStart == 0) {
          buttonBPressStart = currentTime;
        }
        else if (currentTime - buttonBPressStart >= HELP_LONG_PRESS_MS) {

          buttonBHeld = true;

          LOG(LOG_CONTROL, "BtnB 3s — closing sus device view");

          SusDeviceView::close();

          // Open parent view.
          MenuController::open();

          buttonBPressStart = 0;

          // IMPORTANT:
          // The BtnB that closed SusDeviceView must not
          // immediately select something in Main Menu.
          waitForBtnRelease = true;
        }
      }

    } else {

      if (buttonBPressStart > 0 && !buttonBHeld) {

        unsigned long held = currentTime - buttonBPressStart;

        if (held < LONG_PRESS_MS) {

          LOG(LOG_CONTROL, "BtnB short — sus device select (locate)");

          SusDeviceView::selectCurrent();
        }
      }

      buttonBPressStart = 0;
      buttonBHeld = false;
    }

    return;
  }


  // ────────────────────────────────────────────────────────────────
  // Main Menu
  // ────────────────────────────────────────────────────────────────
  if (MenuController::isOpen()) {

    // ── BtnA short = navigate down ─────────────────────────
    if (M5.BtnA.isPressed()) {

      if (!buttonAHeld) {

        if (buttonAPressStart == 0) {
          buttonAPressStart = currentTime;
        }
      }

    } else {

      if (buttonAPressStart > 0 && !buttonAHeld) {

        unsigned long held = currentTime - buttonAPressStart;

        if (held < LONG_PRESS_MS) {

          LOG(LOG_CONTROL, "BtnA short — menu navigate down");

          MenuController::navigateDown();
        }
      }

      buttonAPressStart = 0;
      buttonAHeld = false;
    }


    // ── BtnB short = select/toggle, BtnB 3s = close menu ────
    if (M5.BtnB.isPressed()) {

      if (!buttonBHeld) {

        if (buttonBPressStart == 0) {
          buttonBPressStart = currentTime;
        }
        else if (currentTime - buttonBPressStart >= HELP_LONG_PRESS_MS) {

          buttonBHeld = true;

          LOG(LOG_CONTROL, "BtnB 3s — closing menu");

          MenuController::close();

          buttonBPressStart = 0;

          // Do not let the same physical press
          // trigger anything after the menu closes.
          waitForBtnRelease = true;
        }
      }

    } else {

      if (buttonBPressStart > 0 && !buttonBHeld) {

        unsigned long held = currentTime - buttonBPressStart;

        if (held < LONG_PRESS_MS) {

          LOG(LOG_CONTROL, "BtnB short — menu select/toggle");

          MenuController::selectCurrent();
        }
      }

      buttonBPressStart = 0;
      buttonBHeld = false;
    }

    return;
  }


  // ────────────────────────────────────────────────────────────────
  // Nothing open
  //
  // BtnA 3s = BLE scan
  // BtnB 3s = open menu
  // ────────────────────────────────────────────────────────────────
  if (M5.BtnA.isPressed()) {

    if (!buttonAHeld) {

      if (buttonAPressStart == 0) {
        buttonAPressStart = currentTime;
      }
      else if (currentTime - buttonAPressStart >= HELP_LONG_PRESS_MS) {

        buttonAHeld = true;

        LOG(LOG_CONTROL, "BtnA 3s — BLE scan toggle");

        onLongPress();

        // Protect against the same press being interpreted
        // by a newly opened/changed view.
        waitForBtnRelease = true;
      }
    }

  } else {

    buttonAPressStart = 0;
    buttonAHeld = false;
  }


  #if HAS_TWO_BUTTONS

  // ── BtnB 3s = open menu ──────────────────────────────────
  if (M5.BtnB.isPressed()) {

    if (!buttonBHeld) {

      if (buttonBPressStart == 0) {
        buttonBPressStart = currentTime;
      }
      else if (currentTime - buttonBPressStart >= HELP_LONG_PRESS_MS) {

        buttonBHeld = true;

        LOG(LOG_CONTROL, "BtnB 3s — opening menu");

        MenuController::open();

        buttonBPressStart = 0;

        // VERY IMPORTANT:
        // We opened the menu because of this BtnB press.
        // The menu must not process that same press.
        waitForBtnRelease = true;
      }
    }

  } else {

    buttonBPressStart = 0;
    buttonBHeld = false;
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
    LOG(LOG_CONTROL,"BLE Scan ENABLED");
    if(!MenuController::isOpen() || !SusDeviceView::isOpen() || !FileManagerView::isOpen() || !FinderListView::isOpen() || !ApproachView::isOpen()) {
          drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, 5, 0,
                  nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
    }
    
    delay(1000);
    logNewBoot();
    delay(500);
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
  }
  else {
    LOG(LOG_CONTROL,"BLE Scan DISABLED");
    if(!MenuController::isOpen() || !SusDeviceView::isOpen() || !FileManagerView::isOpen() || !FinderListView::isOpen() || !ApproachView::isOpen()) {
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
    if(!MenuController::isOpen() || !SusDeviceView::isOpen() || !FileManagerView::isOpen() || !FinderListView::isOpen() || !ApproachView::isOpen()) {
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
    if(!MenuController::isOpen() || !SusDeviceView::isOpen() || !FileManagerView::isOpen() || !FinderListView::isOpen() || !ApproachView::isOpen()) {
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
        
        if(!MenuController::isOpen() || !SusDeviceView::isOpen() || !FinderListView::isOpen() || !ApproachView::isOpen() || !FileManagerView::isOpen() ) {
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

        if(!MenuController::isOpen() || !SusDeviceView::isOpen() || !ApproachView::isOpen() || !FinderListView::isOpen() || !FileManagerView::isOpen() ) {
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
