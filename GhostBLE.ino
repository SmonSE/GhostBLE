#include <M5StickCPlus2.h>
#include <M5Cardputer.h>
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

#include "src/globals/globals.h"

#include "src/config/config.h"
#include "src/helper/AvatarHelper.h"
#include "src/sdCard/SDLogger.h"
#include "src/scanner/ScanDevices.h"

#include "src/images/nibblesPcHappyBubble.h"
#include "src/images/nibblesPcSleepingBubble.h"
#include "src/images/nibblesFrontAngry.h"


// Forward declarations of required services/classes
class AvatarHelper;
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
  M5.Lcd.drawBmp(nibblesPcHappyBubble, sizeof(nibblesPcHappyBubble));  // or BITMAP;

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
  delay(5000);
}

void loop() {
  M5Cardputer.update();   // for Cardputer

  unsigned long currentTime = millis();  // ✅ Make sure this line is present

  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (!deviceFound) {
      M5.Lcd.drawBmp(nibblesPcSleepingBubble, sizeof(nibblesPcSleepingBubble));  // or BITMAP;
      scanForDevices();
    } else {
      BLE.stopScan();
      M5.Lcd.drawBmp(nibblesFrontAngry, sizeof(nibblesFrontAngry));  // or BITMAP;
  
      delay(DEVICE_SCAN_TIMEOUT);
      deviceFound = false;
    }

    lastFaceUpdate = currentTime;
  }
}
