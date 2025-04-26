#include <M5StickCPlus2.h>
#include <ArduinoBLE.h>
#include "faces.h"
#include "config.h"

unsigned long lastScanTime = 0;
bool deviceFound = false;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  drawIdleFace();

  if (!BLE.begin()) {
    M5.Lcd.println("Starting BLE failed!");
    while (1);
  }
}

void loop() {
  M5.update();

  unsigned long currentTime = millis();

  if (currentTime - lastScanTime > SCAN_INTERVAL_MS) {
    scanForDevices();
    lastScanTime = currentTime;
  }

  if (deviceFound) {
    drawFoundFace();
  } else {
    drawIdleFace();
  }
}

void scanForDevices() {
  deviceFound = false;
  BLEDevice peripheral = BLE.available();

  while (peripheral) {
    String localName = peripheral.localName();
    if (isTargetDevice(localName)) {
      deviceFound = true;
      return;
    }
    peripheral = BLE.available();
  }
}

bool isTargetDevice(String name) {
  name.toLowerCase();
  return (name.indexOf("bruder") >= 0 ||
          name.indexOf("nemo") >= 0 ||
          name.indexOf("marauder") >= 0 ||
          name.indexOf("cathack") >= 0);
}
