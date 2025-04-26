#include <M5StickCPlus2.h> // Important for the Plus 2!
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include "src/config.h"

unsigned long lastScanTime = 0;
bool deviceFound = false;
unsigned long lastFaceUpdate = 0;
bool isIdle = true;
int top = -50;
int left = -40;

using namespace m5avatar;

Avatar avatar;

//m5avatar::Face face ;     // Default constructor


void setup() {
  M5.begin();
  Serial.begin(115200); // <--- ADD THIS to open Serial Monitor
  delay(500);           // <--- Give Serial Monitor time to open
  
  Serial.println("GhostBLE starting...");

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);

  // Set the face to Neutral at the start
  avatar.init(); // Initialize the avatar
  avatar.setPosition(top, left);
  avatar.setScale(0.8f);  // Set smaller avatar size for the correct display size
  avatar.setExpression(Expression::Neutral);

  if (!BLE.begin()) {
    M5.Lcd.println("Starting BLE failed!");
    Serial.println("BLE initialization failed!");
    while (1); // Halt the program if BLE fails to initialize
  }
  Serial.println("BLE initialized successfully.");
  delay(2000);
}

void loop() {
  M5.update();

  unsigned long currentTime = millis();
  avatar.setPosition(top, left);
  
  // Update the face every second
  if (currentTime - lastFaceUpdate > 1000) {
    if (isIdle) {
      avatar.setExpression(Expression::Happy);
    } else {
      avatar.setExpression(Expression::Angry);
    }
    lastFaceUpdate = currentTime;
  }

  // Call scanForDevices to check for BLE devices
  scanForDevices();
}

void scanForDevices() {
  deviceFound = false;
  BLEDevice peripheral = BLE.available();

  while (peripheral) {
    String localName = peripheral.localName();
    Serial.print("Found device: ");
    Serial.println(localName);

    if (isTargetDevice(localName)) {
      deviceFound = true;
      Serial.println("!!! Target device detected !!!");
      isIdle = false;  // Switch to scanning face if target device is found
      return;
    }
    peripheral = BLE.available();
  }
  isIdle = true; // If no target devices were found, return to idle state
}

bool isTargetDevice(String name) {
  String targetNames[] = {"bruder", "nemo", "marauder", "cathack"};
  name.toLowerCase();
  for (int i = 0; i < sizeof(targetNames) / sizeof(targetNames[0]); i++) {
      if (name.indexOf(targetNames[i]) >= 0) {
          return true;
      }
  }
  return false;
}