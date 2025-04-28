#include <M5StickCPlus2.h>
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include <SD.h>
#include <SPI.h>
#include <vector>
#include "src/config/config.h"
#include "src/helper/ManufacturerHelper.h"
#include "src/helper/ServiceHelper.h"
#include "src/helper/AvatarHelper.h"
#include "src/sdCard/SDLogger.h"

using namespace m5avatar;

AvatarHelper avatarHelper;
SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;

unsigned long lastScanTime = 0;
bool deviceFound = false;
unsigned long lastFaceUpdate = 0;
int top = -40;
int left = -40;
int targetFoundCount = 0;

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
  avatarHelper.update();

  unsigned long currentTime = millis();

  // Update the face every second
  if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
    if (avatarHelper.isAvatarIdle()) {
      avatarHelper.setExpression(Expression::Sleepy);  // Display sleepy face when idle
    } 
    lastFaceUpdate = currentTime;
  }

  if (avatarHelper.isAvatarIdle() && !deviceFound) {
    if (currentTime - lastScanTime > SCAN_INTERVAL_MS) {
      scanForDevices();
      lastScanTime = currentTime;
    }
  } else {
    avatarHelper.setExpression(Expression::Angry);  // Display angry face when scanning
    BLE.stopScan();

    delay(DEVICE_SCAN_TIMEOUT);
    avatarHelper.setIdle(true);
    deviceFound = false;
  }
}

void scanForDevices() {
  deviceFound = false;
  String manuInfo = "";
  String targetMessage = "";
  String serviceInfo = "";
  String localName = "";
  String address = "";

  BLE.scan();
  BLEDevice peripheral = BLE.available();

  while (peripheral) {

    int rssi = 0;

    Serial.println("🔗 Trying to connect for service discovery...");
    if (peripheral.connect()) {
      if (peripheral.discoverAttributes()) {
        Serial.println("✅ Connected and discovered attributes!");

        avatarHelper.setExpression(Expression::Happy);
        delay(2000);

        for (int i = 0; i < peripheral.serviceCount(); i++) {
          targetFoundCount ++;
          localName = peripheral.localName();
          address = peripheral.address();
          rssi = peripheral.rssi();

          BLEService service = peripheral.service(i);
          String serviceUuid = service.uuid();
          serviceUuids.push_back(serviceUuid);
          String serviceNames = getServiceName(serviceUuid);

          serviceInfo = "Discovered Service UUID: " + serviceUuid + " (" + serviceNames + ")";
          Serial.println(serviceInfo);

          Serial.print("Adresse: ");
          Serial.println(address);

          Serial.print("Local Name: ");
          Serial.println(localName);

          if (peripheral.hasManufacturerData()) {
            uint8_t mfgData[64];
            int mfgDataLen = peripheral.manufacturerData(mfgData, sizeof(mfgData));
            if (mfgDataLen >= 2) {
              uint16_t manufacturerId = mfgData[1] << 8 | mfgData[0];
              String manufacturerName = getManufacturerName(manufacturerId);
            
              manuInfo = "Manufacturer ID: 0x" + String(manufacturerId, HEX) + " (" + manufacturerName + ")";
              Serial.println(manuInfo);
            }
          }
          Serial.print("RSSI: ");
          Serial.println(rssi);
        
          float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
          Serial.print("Distanz: ");
          Serial.print(distance, 2);
          Serial.println(" m");

          // CHECK FOR TARGET
          if (isTargetDevice(localName, address, serviceUuid)) {
            deviceFound = true;
            targetMessage = "Target Message: !!! Target detected !!!";
            Serial.println(targetMessage);
            avatarHelper.setIdle(false);
            return;
          } else {
            targetMessage = "Target Message: No Target detected";
            Serial.println(targetMessage);
          }
          Serial.println("-------------------------------");

          sdLogger.writeDeviceInfo(address, localName, manuInfo, targetMessage, serviceInfo);
        }
      } else {
        Serial.println("❌ Attribute discovery failed.");
        avatarHelper.setExpression(Expression::Sleepy);
        delay(100);
      }
      peripheral.disconnect();
      avatarHelper.setExpression(Expression::Sleepy);
      delay(100);
    } else {
      Serial.println("❌ Connection failed.");
      avatarHelper.setExpression(Expression::Sleepy);
      delay(100);
    }
    Serial.println("###############################\n");

    peripheral = BLE.available();
  }

  Serial.print("# Hits: ");
  Serial.println(targetFoundCount);
  Serial.println("\n\n");

  avatarHelper.setIdle(true);
}

bool isTargetDevice(String name, String address, String serviceUuid) {

  // Target detected via mac
  //if (address == "b0:81:84:96:a0:c9") {
  //  Serial.println("🎯 Target erkannt über MAC!");
  //  return true;
  //}

  // Target detected via name
  //if (name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0") {
  //  Serial.print("Detected Name: ");
  //  Serial.println(name);
  //  Serial.println("⚠ Detected device with generic or no name. Possible M5 HW?");
  //  return true;
  //}

  // Target detected via service uuid 128-big
  if (serviceUuid == TARGET_SERVICE_UUID) {
    Serial.println("🎯 Target erkannt über spezielle 128-bit Service UUID!");
    return true;
  }

  // Target detected via keyboard uuid 16-bit
  if (serviceUuid == BRUCE_KEYBOARD_UUID) {
    Serial.println("🎯 Target erkannt über spezielle 16-bit Service UUID!");
    return true;
  }

  return false;
}