#include <M5StickCPlus2.h> // Important for the Plus 2!
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include "src/config.h"

unsigned long lastScanTime = 0;
bool deviceFound = false;
unsigned long lastFaceUpdate = 0;
bool isIdle = true;

using namespace m5avatar;

Avatar avatar;

m5avatar::Face idleFace;     // Default constructor
m5avatar::Face scanningFace; // Default constructor

void setup() {
  M5.begin();
  Serial.begin(115200); // <--- ADD THIS to open Serial Monitor
  delay(500);           // <--- Give Serial Monitor time to open
  Serial.println("GhostBLE starting...");

  M5.begin();
  avatar.init(); // Initialize the avatar

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);

  int top = -10;
  int left = -10;
  avatar.setPosition(top, left);
  avatar.setScale(0.8f);  // Set smaller avatar size for the correct display size

  // Set the face to Idle at the start
  avatar.setFace(&idleFace);  // Set the face to Idle

  if (!BLE.begin()) {
    M5.Lcd.println("Starting BLE failed!");
    Serial.println("BLE initialization failed!");
    while (1); // Halt the program if BLE fails to initialize
  }
  Serial.println("BLE initialized successfully.");
}

void loop() {
  M5.update();

  unsigned long currentTime = millis();
  
  // Update the face every second
  if (currentTime - lastFaceUpdate > 1000) {
    if (isIdle) {
      int top = -40;
      int left = -30;
      avatar.setPosition(top, left);
      avatar.setFace(&idleFace);  // Set the face to Idle when idle
    } else {
      avatar.setFace(&scanningFace);  // Set the face to Scanning when searching for devices
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
  name.toLowerCase();
  return (name.indexOf("bruder") >= 0 ||
          name.indexOf("nemo") >= 0 ||
          name.indexOf("marauder") >= 0 ||
          name.indexOf("cathack") >= 0);
}
