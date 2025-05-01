#include <M5StickCPlus2.h>
#include <ArduinoBLE.h>
#include <M5Unified.h>
#include <Avatar.h>  // Ensure this is the correct path to your Avatar library
#include <SD.h>
#include <SPI.h>
#include <vector>

#include "src/globals/globals.h"

#include "src/config/config.h"
#include "src/helper/ManufacturerHelper.h"
#include "src/helper/ServiceHelper.h"
#include "src/helper/AvatarHelper.h"
#include "src/sdCard/SDLogger.h"
#include "src/bleServices/deviceInfoService.h"
#include "src/bleServices/heartRateService.h"
#include "src/bleServices/batteryLevelService.h"
#include "src/bleServices/currentTimeService.h"


using namespace m5avatar;

AvatarHelper avatarHelper;
SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;

unsigned long lastScanTime = 0;
unsigned long lastFaceUpdate = 0;
int targetFoundCount = 0;
bool isHappyTaskRunning = false;
bool isAngryTaskRunning = false;

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

void scanForDevices() {

  BLE.scan();
  BLEDevice peripheral = BLE.available();

  while (peripheral) {
    int rssi = 0;

    Serial.println("🔗 Trying to connect for service discovery...");
    delay(500);
    if (peripheral.connect()) {
      if (peripheral.discoverAttributes()) {
        Serial.println("✅ Connected and discovered attributes!");

        if (!isHappyTaskRunning) {
          xTaskCreate(showHappyExpressionTask, "HappyFace", 2048, NULL, 1, NULL);
        }
        targetFoundCount++;

        address = peripheral.address();
        Serial.print("Adresse: ");
        Serial.println(address);

        // Call deviceInfoService
        deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(peripheral);

        // Call heartRateService
        heartRateService = HeartRateServiceHandler::readHeartRate(peripheral);

        // Call batteryLevelService
        batteryLevelService = BatteryServiceHandler::readBatteryLevel(peripheral);

        // Call timeInfo
        timeInfoService = CurrentTimeServiceHandler::readCurrentTime(peripheral);


        if (peripheral.hasManufacturerData()) {
          hasManuData = true;
          uint8_t mfgData[64];
          int mfgDataLen = peripheral.manufacturerData(mfgData, sizeof(mfgData));
          if (mfgDataLen >= 2) {
            uint16_t manufacturerId = mfgData[1] << 8 | mfgData[0];
            String manufacturerName = getManufacturerName(manufacturerId);
            manuInfo = "Manufacturer ID: 0x" + String(manufacturerId, HEX) + " (" + manufacturerName + ")";
            Serial.println(manuInfo);

            if (isIgnoredManufacturer(manufacturerId)) {
              Serial.println("Ignore Manufacturer.");
              skipLogging = true;
            }
          } else {
            hasManuData = false;
            manuInfo = "Manufacturer ID: ";
            Serial.println(manuInfo);
          }
        }

        // MAIN UUID
        if (peripheral.serviceCount() > 0) {
          BLEService mainService = peripheral.service(0);
          const char* mainUuid = mainService.uuid();
          mainUuidStr = String(mainUuid);

          Serial.print("Primary UUID: ");
          Serial.println(mainUuidStr);
        }

        char serviceUuid[64];
        for (int i = 0; i < peripheral.serviceCount(); i++) {
          BLEService service = peripheral.service(i);
          strncpy(serviceUuid, service.uuid(), sizeof(serviceUuid));
          serviceUuid[sizeof(serviceUuid) - 1] = '\0';

          serviceInfo = String("Service UUID: ") + serviceUuid;
          Serial.println(serviceInfo);

          // CHECK FOR TARGET
          if (isTargetDevice(localName, address, serviceUuid, hasManuData)) {
            deviceFound = true;
            targetMessage = "Target Message: !!! Target detected !!!";
            Serial.println(targetMessage);
            avatarHelper.setIdle(false);
            return;
          } else {
            //targetMessage = "Target Message: No Target detected";
            //Serial.println(targetMessage);
          }
        }

        localName = peripheral.localName();
        Serial.print("Local Name: ");
        Serial.println(localName);

        rssi = peripheral.rssi();
        Serial.print("RSSI: ");
        Serial.println(rssi);

        float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
        Serial.print("Distanz: ");
        Serial.print(distance, 2);
        Serial.println(" m");


        // Only write if not skipped 
        if (!skipLogging) {
          sdLogger.writeDeviceInfo(address, localName, manuInfo, targetMessage, mainUuidStr, deviceInfoService);
        } else {
          Serial.println("Skip logging.");
        }
      } else {
        Serial.println("Attribute discovery failed.");
      }
      deviceInfoService = ""; // delete for next scan
      peripheral.disconnect();
      if (!isAngryTaskRunning) {
        avatarHelper.setExpression(Expression::Sleepy);
      }
    } else {
      avatarHelper.setExpression(Expression::Sleepy);
      Serial.println("Connection failed.");
    }
    Serial.println("###############################\n");

    peripheral = BLE.available();
  }

  Serial.print("# Hits: ");
  Serial.println(targetFoundCount);
  Serial.println("\n\n");

  avatarHelper.setIdle(true);
}


bool isTargetDevice(String name, String address, String serviceUuid, bool hasManuData) {
  // Target detected via mac
  //if (address == "b0:81:84:96:a0:c9") {
  //  Serial.println("🎯 Target erkannt über MAC!");
  //  return true;
  //}

  // 0. CATHACK-Signatur
  if ((name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0 "|| name == "Keyboard_a9")) {
    Serial.println("🎯 Device with ESP32 Hardware detected");
    return true;
  }


  // 1. LIGHTBLUE APP UUIDs
  if (serviceUuid == LIGHTBLUE_APP_SERVICE_UUID) {
    Serial.println("🎯 Device with LIGHT BLUE APP detected");
    return true;
  }

  // 2. CATHACK-Signatur
  if ((serviceUuid == CATHACK_SERVICE_UUID_5 ||
       serviceUuid == CATHACK_SERVICE_UUID_6) && 
       (name == "esp32" || name == "n/a" || name == "<no name>" || name == "Keyboard_a0")) {
    Serial.println("🎯 Device with CATHACK Firmware detected");
    return true;
  }

  // 3. FLIPPER ZERO UUIDs
  if (serviceUuid == FLIPPER_BLACK_UUID) {
    Serial.println("🐬 FLIPPER ZERO detected (Black)");
    return true;
  }
  if (serviceUuid == FLIPPER_WHITE_UUID) {
    Serial.println("🐬 FLIPPER ZERO detected (White)");
    return true;
  }
  if (serviceUuid == FLIPPER_TRANSPARENT_UUID) {
    Serial.println("🐬 FLIPPER ZERO detected (Transparent)");
    return true;
  }

  return false;
}


void showHappyExpressionTask(void* parameter) {
  isHappyTaskRunning = true;
  isAngryTaskRunning = true;
  avatarHelper.setExpression(Expression::Happy);
  vTaskDelay(pdMS_TO_TICKS(3000));  // 3 Sekunden
  //avatarHelper.setExpression(Expression::Sleepy);
  isHappyTaskRunning = false;
  isAngryTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}
