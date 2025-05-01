#include <M5StickCPlus2.h>
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include <SD.h>
#include <SPI.h>
#include <vector>

#include "src/globals/globals.h"

#include "src/config/config.h"
#include "src/helper/AvatarHelper.h"
#include "src/sdCard/SDLogger.h"
#include "src/scanner/ScanDevices.h"


using namespace m5avatar;

// Forward declarations of required services/classes
class AvatarHelper;
class SDLogger;

// External global instances
extern AvatarHelper avatarHelper;
extern SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;


void setup() {
  M5.begin();
  Serial.begin(115200);
  delay(500);

  Serial.println("GhostBLE starting...");

  #if defined(STICK_C_PLUS2)
    M5.Lcd.setRotation(3);
  #endif

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(BLACK);

  avatarHelper.init();
  avatarHelper.setExpression(Expression::Neutral);
  avatarHelper.setIdle(true);

  if (!BLE.begin()) {
    M5.Lcd.println("Starting BLE failed!");
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
  M5.update();
  unsigned long currentTime = millis();

  avatarHelper.update();

  // Update the face every second
  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (avatarHelper.isAvatarIdle() && !deviceFound) {
      avatarHelper.setExpression(Expression::Sleepy);  // Display sleepy face when idle
      scanForDevices();
    } else {
      BLE.stopScan();
      avatarHelper.setIdle(false);
      avatarHelper.setExpression(Expression::Angry);  // Display angry face when scanning
  
      delay(DEVICE_SCAN_TIMEOUT);
      avatarHelper.setIdle(true);
      deviceFound = false;
    }

    lastFaceUpdate = currentTime;
  }
}
