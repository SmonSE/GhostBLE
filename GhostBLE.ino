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

#include "src/images/nibblesStartWorking.h"
#include "src/images/nibblesFront.h"
#include "src/images/nibblesGlasses.h"
#include "src/images/nibblesAngry.h"
#include "src/images/nibblesSad.h"
#include "src/images/nibblesHeartLeft.h"
#include "src/images/nibblesHeartRight.h"


unsigned long startTimeDevice;
const unsigned long timerDurationDevice = 10 * 60 * 1000; // 10 Minuten in Millisekunden

// Forward declarations of required services/classes
class SDLogger;

// External global instances
extern SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;


void setup() {
  M5Cardputer.begin();   // for Cardputer
  Serial.begin(115200);
  delay(500);

  Serial.println("GhostBLE starting...");

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(BLACK);    // Optional

  M5.Lcd.drawBmp(nibblesStartWorking, sizeof(nibblesStartWorking));  
  delay(5000);

  NimBLEDevice::init("");

  #if defined(CARDPUTER)
  if (!sdLogger.begin(SD_CS_PIN)) {
    while (1);  // Halt if SD card init fails
  }
  #endif

  Serial.println("BLE initialized successfully.");

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);

  showFindingCounter(targetConnects, susDevice, spottedDevice);
  delay(1000);

  startTimeDevice = millis(); // Startzeit speichern

  scanIsRunning = false;
  // OVERLAY WITH LETS WORK BUBBLE
}

void loop() {
  M5Cardputer.update();   // for Cardputer

  unsigned long currentTime = millis();  // ✅ Make sure this line is present

  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (!targetFound && !scanIsRunning) {
      scanForDevices();
    } else {
      targetFound = false;
    }
    lastFaceUpdate = currentTime;
  }

  unsigned long currentTimeDevice = millis();
  if (currentTimeDevice - startTimeDevice >= timerDurationDevice) {
    Serial.println("10 Minuten sind vorbei!");
    if (!seenDevices.empty()) {
      seenDevices.clear();
      Serial.println("CLEAR SEEN DEVICES");
    } else {
      Serial.println("SEEN DEVICES STILL EMPTY");
    }

    startTimeDevice = millis();
  }
}

