#include <M5StickCPlus2.h>
#include <M5Cardputer.h>
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
  M5Cardputer.update();   // for Cardputer

  unsigned long currentTime = millis();  // ✅ Make sure this line is present

  // Debugging: Print whether the button press is detected
  if (M5Cardputer.BtnA.wasPressed()) {
    BLE.stopScan();
    Serial.println("Button A was pressed!");
    btn0pressed = true;
    showLastConnectedDevice();  // Call your function here
  }

  // Manage button release state with extra debugging
  if (btn0pressed) {
    Serial.println("Button A released!");
    btn0pressed = false;
  }

  // Update other tasks (e.g., avatar, background tasks)
  avatarHelper.update();

  // Handle avatar state update (sleepy/angry) and scanning logic
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
