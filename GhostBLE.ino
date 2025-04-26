#include <M5StickCPlus2.h> // Important for the Plus 2!
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include "src/config.h"

unsigned long lastScanTime = 0;
bool deviceFound = false;
unsigned long lastFaceUpdate = 0;
bool isIdle = false;
int top = -40;
int left = -40;

int targetFoundCount = 0; // <- Zähler für gefundene Geräte


using namespace m5avatar;

Avatar avatar;


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
  isIdle = true;

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
    } 
    lastFaceUpdate = currentTime;
  }

  // Check if it's time to scan
  if (isIdle && !deviceFound) {
    if (currentTime - lastScanTime > SCAN_INTERVAL_MS) {
      scanForDevices();
      lastScanTime = currentTime;  // Update last scan timestamp
    }
  }
  else {
      avatar.setExpression(Expression::Angry);  // Display angry face when scanning
      BLE.stopScan();  // Stop scanning when face is angry

      delay(20000);
      isIdle = true;
      deviceFound = false;
  }
}

void scanForDevices() {
  deviceFound = false;
  
  BLE.scan();  // Start scanning
  BLEDevice peripheral = BLE.available();

  while (peripheral) {
    //Serial.println("🔍 Scanning device...");
    
    String localName = peripheral.localName();
    String address = peripheral.address();
    int rssi = peripheral.rssi();
    
    Serial.print("🔎 Adresse: ");
    Serial.println(address);

    Serial.print("📛 Local Name: ");
    Serial.println(localName);

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
          targetFoundCount++; // <-- Zähler erhöhen!
          deviceFound = true;  // Set device found flag
          break;  // Exit loop if target UUID is found
        }

        // Check for specific MAC address of Nemo device
        String targetMacAddress = "b0:81:84:96:a0:c9"; 
        if (address == targetMacAddress) {
          Serial.println("🎯 Found NEMO device via MAC address!");
          targetFoundCount++;
          deviceFound = true;
          break; // Exit loop if target UUID is found
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
    Serial.println("-------------------------------");

    // Check if it’s a target device
    if (isTargetDevice(localName, address)) {
      deviceFound = true;
      Serial.println("🎯 !!! Target device detected !!!");
      Serial.println("-------------------------------");
      isIdle = false;  // Device found, so not idle

      return;  // Exit loop when target device is found
    }

    peripheral = BLE.available();
  }

  Serial.print("# Hits: ");
  Serial.println(targetFoundCount);
  Serial.println("\n\n");

  isIdle = true;  // Set back to idle if no target device is found
}


bool isTargetDevice(String name, String address) {
  // Target MAC addresses (you can add more if needed)
  String targetMacAddresses[] = {
    "b0:81:84:96:a0:c9", // Your Nemo MAC address
    // Add more known addresses here if you want
  };

  // Check by MAC address
  for (int i = 0; i < sizeof(targetMacAddresses) / sizeof(targetMacAddresses[0]); i++) {
    if (address.equalsIgnoreCase(targetMacAddresses[i])) {
      return true;
    }
  }

  // Optionally: check for generic/empty names
  // Some Nemos might show no name or very generic names like "ESP32" or "N/A"
  //name.toLowerCase();
  //if (name == "" || name == "esp32" || name == "n/a" || name == "<no name>") {
  //  Serial.println("⚠ Detected device with generic or no name. Possible Nemo?");
  //  return true;
  //}

  return false;
}

