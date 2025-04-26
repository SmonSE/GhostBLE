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
  delay(3000);
}

void loop() {
  M5.update();

  unsigned long currentTime = millis();
  
  avatar.setPosition(top, left);

  // Update the face every second
  if (currentTime - lastFaceUpdate > 1000) {
    if (isIdle) {
      avatar.setExpression(Expression::Sleepy);  // Display sleepy face when idle
      Serial.println("🛏️ Face is Sleepy (Idle)");
    } else {
      avatar.setExpression(Expression::Angry);  // Display angry face when scanning
      Serial.println("😡 Face is Angry (Device found)");
      BLE.stopScan();  // Stop scanning when face is angry
    }
    lastFaceUpdate = currentTime;
  }

  // Check if it's time to scan
  if (currentTime - lastScanTime > SCAN_INTERVAL_MS && isIdle) {
    scanForDevices();
    lastScanTime = currentTime;  // Update last scan timestamp
  }
}

void scanForDevices() {
  deviceFound = false;
  
  BLE.scan();  // Start scanning
  BLEDevice peripheral = BLE.available();

  while (peripheral) {
    Serial.println("🔍 Scanning device...");
    
    String localName = peripheral.localName();
    String address = peripheral.address();
    int rssi = peripheral.rssi();
    
    Serial.print("🔎 Adresse: ");
    Serial.println(address);

    // Check for advertised service UUIDs
    int advServiceCount = peripheral.advertisedServiceUuidCount();
    if (advServiceCount > 0) {
      for (int i = 0; i < advServiceCount; i++) {
        String serviceUuid = peripheral.advertisedServiceUuid(i);
        Serial.print("📦 Service UUID: ");
        Serial.println(serviceUuid);

        // Check for the 1812 service UUID
        if (serviceUuid == "1812") {
          Serial.println("🎯 Found target service UUID (1812)!");
          deviceFound = true;  // Set device found flag
          break;  // Exit loop if target UUID is found
        }
      }
    } else {
      Serial.println("⚠ No Service UUIDs found!");
    }

    Serial.print("📶 RSSI: ");
    Serial.println(rssi);

    // Optional: estimate distance from RSSI
    float distance = pow(10, (-69 - rssi) / 20.0); 
    Serial.print("📏 Distanz: ");
    Serial.print(distance, 2);
    Serial.println(" m");

    // Print number of advertised service UUIDs
    Serial.print("📦 Service UUIDs Found: ");
    Serial.println(advServiceCount);

    // Check if it’s a target device
    if (isTargetDevice(localName, address)) {
      deviceFound = true;
      Serial.println("🎯 !!! Target device detected !!!");
      isIdle = false;  // Device found, so not idle
      
      return;  // Exit loop when target device is found
    }

    peripheral = BLE.available();
  }
  Serial.println("\n\n");

  isIdle = true;  // Set back to idle if no target device is found
}


bool isTargetDevice(String name, String address) {
  // Check if the address starts with Espressif's known MAC address prefixes
  String esp32Prefixes[] = {"30:AE:A4", "24:0A:C4", "F4:CE:46"};
  
  for (int i = 0; i < sizeof(esp32Prefixes) / sizeof(esp32Prefixes[0]); i++) {
    if (address.startsWith(esp32Prefixes[i])) {
      return true;
    }
  }

  // If the name is empty, use the address to identify the device
  if (name.isEmpty()) {
    return address.startsWith("B0:81:84");
  }

  // Check based on device name
  String targetNames[] = {"bruder", "nemo", "marauder", "cathack", "<no name>", "ESP32"};
  name.toLowerCase();
  for (int i = 0; i < sizeof(targetNames) / sizeof(targetNames[0]); i++) {
    if (name.indexOf(targetNames[i]) >= 0) {
      return true;
    }
  }

  // Check based on known OUIs (MAC prefixes)
  String ouiPrefixes[] = {
    "E7:6F:8C", "F4:CE:46", "D0:39:72",  // Nordic
    "24:0A:C4", "30:AE:A4", "84:0D:8E"   // Espressif
  };
  
  address.toUpperCase(); // Just to be safe
  for (int i = 0; i < sizeof(ouiPrefixes) / sizeof(ouiPrefixes[0]); i++) {
    if (address.startsWith(ouiPrefixes[i])) {
      return true;
    }
  }

  return false;
}
