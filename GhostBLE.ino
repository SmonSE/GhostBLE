#include <M5StickCPlus2.h>
#include <M5Cardputer.h>
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

#include "src/globals/globals.h"

#include "src/config/config.h"
#include "src/sdCard/SDLogger.h"
#include "src/scanner/ScanDevices.h"
#include "src/helper/drawOverlay.h"

#include "src/images/nibblesStartWorking.h"
#include "src/images/nibblesFrontHappy.h"
#include "src/images/nibblesGlasses.h"
#include "src/images/nibblesAngry.h"
#include "src/images/nibblesSad.h"
#include "src/images/nibblesHeartLeft.h"
#include "src/images/nibblesHeartRight.h"
#include "src/images/nibblesPawnd.h"

unsigned long startTimeDevice;
const unsigned long timerDurationDevice = 5 * 60 * 1000; // 5 Minuten in Millisekunden

// Forward declarations of required services/classes
class SDLogger;

// External global instances
extern SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;


void setup() {
  M5Cardputer.begin();   // for Cardputer
  //M5.begin();          // for M5Stick
  Serial.begin(115200);
  delay(500);

  Serial.println("GhostBLE starting...");

  #if defined(STICK_C_PLUS2)
    M5.Lcd.setRotation(3);
  #endif

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(BLACK);    // Optional

  M5.Lcd.drawBmp(nibblesStartWorking, sizeof(nibblesStartWorking));  
  delay(5000);

  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);  // Halt the program if BLE fails
  }

  #if defined(CARDPUTER)
  if (!sdLogger.begin(SD_CS_PIN)) {
    while (1);  // Halt if SD card init fails
  }
  #endif

  Serial.println("BLE initialized successfully.");

  M5.Lcd.drawBmp(nibblesFrontHappy, sizeof(nibblesFrontHappy));
  showFindingCounter(targetConnects, spottedDevice);
  delay(2000);

  startTimeDevice = millis(); // Startzeit speichern

  // OVERLAY WITH LETS WORK BUBBLE
}

void loop() {
  M5Cardputer.update();   // for Cardputer

  unsigned long currentTime = millis();  // ✅ Make sure this line is present

  delay(500);
  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (!targetFound) {
      delay(500);
      scanForDevices();
    } else {
      BLE.stopScan();
      if (!isAngryTaskRunning) {
        Serial.println("showAngryExpressionTask");
        xTaskCreate(showAngryExpressionTask, "AngryFace", 2048, NULL, 1, NULL);
      }
      delay(DEVICE_SCAN_TIMEOUT);
      targetFound = false;
    }

    lastFaceUpdate = currentTime;
  }

  unsigned long currentTimeDevice = millis();
  if (currentTimeDevice - startTimeDevice >= timerDurationDevice) {
    Serial.println("5 Minuten sind vorbei!");
    seenDevices.clear();
    Serial.println("CLEAR SEEN DEVICES");
    startTimeDevice = millis();
  }
}

