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

// Forward declarations of required services/classes
class SDLogger;

// External global instances
extern SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;

bool btn0pressed = false;
unsigned long btn0pressTime = 0;
const unsigned long displayDuration = 5000; // 5 seconds

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

  // 80 = 80 Pixel von links
  // 40 = 40 Pixel von oben
  //drawOverlay(nibblesHeartLeft, NIBBLESHEARTLEFT_WIDTH, NIBBLESHEARTLEFT_HEIGHT, 10, 10);
  //delay(1000);
  //drawOverlay(nibblesHeartRight, NIBBLESHEARTRIGHT_WIDTH, NIBBLESHEARTRIGHT_HEIGHT, 160, 12);
  //drawOverlay(nibblesGlasses, NIBBLESGLASSES_WIDTH, NIBBLESGLASSES_HEIGHT, 77, 52);
  //drawOverlay(nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 77, 42);
  //M5.Lcd.drawBmp(nibblesFrontHappy, sizeof(nibblesFrontHappy));
  //drawOverlay(nibblesAngry, NIBBLESANGRY_WIDTH, NIBBLESANGRY_HEIGHT, 77, 42);
  //drawOverlay(nibblesPawnd, NIBBLESPAWND_WIDTH, NIBBLESPAWND_HEIGHT, 77, 49);

}

void loop() {
  M5Cardputer.update();   // for Cardputer

  unsigned long currentTime = millis();  // ✅ Make sure this line is present

  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (!deviceFound) {
      delay(500);
      M5.Lcd.drawBmp(nibblesFrontHappy, sizeof(nibblesFrontHappy));
      scanForDevices();
    } else {
      if (!isAngryTaskRunning) {
        xTaskCreate(showGlassesExpressionTask, "AngryFace", 2048, NULL, 0, NULL);
      }
      BLE.stopScan();
      delay(DEVICE_SCAN_TIMEOUT);
      deviceFound = false;
    }

    lastFaceUpdate = currentTime;
  }
}
