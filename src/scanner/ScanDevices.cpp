#include <M5Cardputer.h>

#include "ScanDevices.h"
#include "../globals/globals.h"
#include "../helper/ManufacturerHelper.h"
#include "../helper/ServiceHelper.h"
#include "../helper/drawOverlay.h"

#include "../images/nibblesFrontHappy.h"
#include "../images/nibblesFront.h"
#include "../images/nibblesGlasses.h"
#include "../images/nibblesAngry.h"
#include "../images/nibblesSad.h"
#include "../images/nibblesHeartLeft.h"
#include "../images/nibblesHeartRight.h"
#include "../images/nibblesPawnd.h"

// Forward declarations of required services/classes
class SDLogger;
SDLogger sdLogger;


void scanForDevices() {
  BLE.scan();
  BLEDevice peripheral = BLE.available();

  while (peripheral) {
    int rssi = 0;
    String localName;
    String address;
    String manuInfo = "";
    String targetMessage = "";
    String mainUuidStr = "";
 
    bool skipLogging = false;
    targetFound = false;
    hasManuData = false;

    Serial.println("🔗 Trying to connect for service discovery...");
    delay(500);
    
    if (peripheral.connect()) {
      if (peripheral.discoverAttributes()) {
        Serial.println("✅ Connected and discovered attributes!");

        if (!isGlassesTaskRunning && !isAngryTaskRunning) {
          xTaskCreate(showGlassesExpressionTask, "HappyFace", 2048, NULL, 0, NULL);
        }

        targetFoundCount++;
        address = peripheral.address();
        Serial.print("Adresse: ");
        Serial.println(address);

        // Service calls
        deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(peripheral);
        genericAccessInfo = DeviceInfoServiceHandler::readGenericAccessInfo(peripheral);
        //heartRateService = HeartRateServiceHandler::readHeartRate(peripheral);
        //batteryLevelService = BatteryServiceHandler::readBatteryLevel(peripheral);
        //timeInfoService = CurrentTimeServiceHandler::readCurrentTime(peripheral);

        // Manufacturer handling
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

        // Main UUID
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

          String serviceInfo = "Service UUID: ";
          serviceInfo += serviceUuid;
          Serial.println(serviceInfo);

          localName = peripheral.localName();

          if (isTargetDevice(localName, address, serviceUuid, hasManuData)) {
            targetFound = true;
            targetMessage = "Target Message: !!! Target detected !!!";
            Serial.println(targetMessage);
            break;  // break for loop
          }
        }
        // Break WHILE LOOP
        if (targetFound) {
          Serial.println("ScanDevices: Found Device -> break");
          break;
        }

        // Print device info
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

        // Log to SD
        if (!skipLogging) {
          sdLogger.writeDeviceInfo(address, localName, manuInfo, targetMessage, mainUuidStr, deviceInfoService);
        } else {
          Serial.println("Skip logging.");
        }
      } else {
        Serial.println("Attribute discovery failed.");
      }

      deviceInfoService = "";
      genericAccessInfo = "";
      peripheral.disconnect();

    } else {
      Serial.println("Connection failed.");
    }

    Serial.println("###############################\n");
    peripheral = BLE.available();
  }

  Serial.print("# Hits: ");
  Serial.println(targetFoundCount);
  Serial.println("\n\n");
}

void showLastConnectedDevice() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE);
  
  M5.Lcd.println("Letztes verbundenes Gerät:");
  M5.Lcd.println("----------------------------");
  M5.Lcd.println(lastConnectedDeviceInfo);

  delay(5000);  // Zeige 5 Sekunden lang an
  M5.Lcd.fillScreen(BLACK);
}

void showGlassesExpressionTask(void* parameter) {
    isGlassesTaskRunning = true;
    drawOverlay(nibblesGlasses, NIBBLESGLASSES_WIDTH, NIBBLESGLASSES_HEIGHT, 77, 52);
    vTaskDelay(pdMS_TO_TICKS(2000));  // 3 Sekunden
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    isGlassesTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}

void showAngryExpressionTask(void* parameter) {
  isAngryTaskRunning = true;
  drawOverlay(nibblesAngry, NIBBLESANGRY_WIDTH, NIBBLESANGRY_HEIGHT, 77, 42);
  vTaskDelay(pdMS_TO_TICKS(4000));  // 3 Sekunden
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  isAngryTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}