#include <NimBLEDevice.h>
#include <M5Cardputer.h>
#include <set>

#include "ScanDevices.h"
#include "../globals/globals.h"
#include "../helper/ManufacturerHelper.h"
#include "../helper/ServiceHelper.h"
#include "../helper/drawOverlay.h"

#include "../images/nibblesFront.h"
#include "../images/nibblesGlasses.h"
#include "../images/nibblesAngry.h"
#include "../images/nibblesSad.h"
#include "../images/nibblesHeartLeft.h"
#include "../images/nibblesHeartRight.h"


NimBLEScan* pBLEScan = nullptr;

// Forward declarations of required services/classes
class SDLogger;
SDLogger sdLogger;


#define MAX_SEEN_DEVICES 1000


void scanForDevices() {
  NimBLEScan* pScan = NimBLEDevice::getScan();
  
  if (pScan == nullptr) {
    Serial.println("Scan instance creation failed.");
    return;  // Early exit if pScan is null
  }

  pScan->setActiveScan(true);  // Set active scan mode
  pScan->setInterval(100);
  pScan->setWindow(100);
  pScan->start(10, false);     // Start scanning for 10 seconds

  delay(1000);  // Add a small delay to ensure scan results are populated

  NimBLEScanResults results = pScan->getResults();  // Get scan results
  if (results.getCount() == 0) {
    //Serial.println("No devices found.");
  } else {
    for (int i = 0; i < results.getCount(); ++i) {
      const NimBLEAdvertisedDevice* device = results.getDevice(i);
      String address = device->getAddress().toString().c_str();
      String localName = String(device->getName().c_str());
      int rssi = device->getRSSI();

      if (seenDevices.empty()) {
        Serial.println("🧪 seenDevices is currently empty");
      }

      Serial.printf("🧪 seenDevices size: %d\n", seenDevices.size());
      Serial.printf("🧪 Trying to access address: %s\n", address.c_str());      

      try {
        if (seenDevices.find(std::string(address.c_str())) != seenDevices.end()) {
          Serial.print("🛑 Bereits gesehen: ");
          Serial.println(address.c_str());
          continue;
        }
      
        seenDevices.insert(std::string(address.c_str()));
      
        if (seenDevices.size() >= MAX_SEEN_DEVICES) {
          seenDevices.clear();
        }
      } catch (...) {
        Serial.println("⚠️ Exception caught accessing seenDevices");
      }
      

      Serial.println("🔗 Trying to connect for service discovery...");
      delay(1000);
  
      // Create client to connect to the device
      NimBLEClient* pClient = NimBLEDevice::createClient();
      if (pClient == nullptr) {
        Serial.println("Client creation failed.");
        delay(1000);
        if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
          Serial.println("showSadExpressionTask");
          xTaskCreate(showSadExpressionTask, "SadFace", 2048, NULL, 1, NULL);
        }
        continue;  // Skip this device and move to the next one
      }
      
      if (pClient->connect(device)) {
        if (pClient->discoverAttributes()) {
          Serial.println("✅ Connected and discovered attributes!");

          targetConnects++;

          if (!isGlassesTaskRunning && !isAngryTaskRunning) {
            xTaskCreate(showGlassesExpressionTask, "HappyFace", 2048, NULL, 0, NULL);
          }

          Serial.print("Adresse: ");
          Serial.println(address);

          deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);
          batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
          //heartRateService = HeartRateServiceHandler::readHeartRate(pClient);

          // Manufacturer handling
          String manuInfo = "";
          if (device->haveManufacturerData()) {
            std::string mfg = device->getManufacturerData();
            Serial.print("Manufacturer Data: ");
            Serial.println(mfg.c_str());

            uint16_t manufacturerId = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
            String manufacturerName = getManufacturerName(manufacturerId);
            manuInfo = "Manufacturer ID: 0x" + String(manufacturerId, HEX) + " (" + manufacturerName + ")";
            Serial.println(manuInfo);

            if (isIgnoredManufacturer(manufacturerId)) {
              Serial.println("Ignore Manufacturer.");
              pClient->disconnect();
              NimBLEDevice::deleteClient(pClient);
              continue;
            }
          }

          std::string mainUuidStr = "";
          if (pClient->getServices().size() > 0) {
            NimBLERemoteService* mainService = pClient->getServices()[0];
            mainUuidStr = mainService->getUUID().toString();
            Serial.print("Primary UUID: ");
            Serial.println(mainUuidStr.c_str());
          }

          bool isTarget = false;
          for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
            NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
            std::string serviceUuid = service->getUUID().toString();
            Serial.print("Service UUID: ");
            Serial.println(serviceUuid.c_str());
      
            std::string localName = device->getName();
            if (isTargetDevice(String(localName.c_str()), String(address.c_str()), String(serviceUuid.c_str()))) {
              targetFound = true;
              susDevice++;
              Serial.println("Target Message: !!! Target detected !!!");
              delay(2000);
              if (!isAngryTaskRunning) {
                Serial.println("showAngryExpressionTask");
                xTaskCreate(showAngryExpressionTask, "AngryFace", 2048, NULL, 2, NULL);
              }
              isTarget = true;
              break;
            }
          }
      
          if (isTarget) {
            Serial.println("ScanDevices: Found Device -> break");
            pClient->disconnect();
            NimBLEDevice::deleteClient(pClient);
            break;
          }

          // Print device info
          Serial.print("Local Name: ");
          Serial.println(localName);

          Serial.print("RSSI: ");
          Serial.println(rssi);

          float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
          Serial.print("Distanz: ");
          Serial.print(distance, 2);
          Serial.println(" m");

          // Log to SD card
          if (!manuInfo.isEmpty()) {
            sdLogger.writeDeviceInfo(address, localName, manuInfo, targetMessage, String(mainUuidStr.c_str()), deviceInfoService, genericAccessInfo, batteryLevelService);
          } else {
            Serial.println("Skip logging.");
          }

        } else {
          Serial.println("Attribute discovery failed.");
        }

        pClient->disconnect();
        NimBLEDevice::deleteClient(pClient);
      }
      Serial.println("###############################\n");
    }
  }

}


void showGlassesExpressionTask(void* parameter) {
  isGlassesTaskRunning = true;
  drawOverlay(nibblesGlasses, NIBBLESGLASSES_WIDTH, NIBBLESGLASSES_HEIGHT, 77, 52);
  vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);

  int getSpottedDevice = seenDevices.size();
  spottedDevice = getSpottedDevice;
  showFindingCounter(targetConnects, susDevice, spottedDevice);  

  isGlassesTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}


void showAngryExpressionTask(void* parameter) {
  isAngryTaskRunning = true;
  drawOverlay(nibblesAngry, NIBBLESANGRY_WIDTH, NIBBLESANGRY_HEIGHT, 77, 42);
  vTaskDelay(pdMS_TO_TICKS(3000));  // 3 Sekunden
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);

  int getSpottedDevice = seenDevices.size();
  spottedDevice = getSpottedDevice;
  showFindingCounter(targetConnects, susDevice, spottedDevice);  

  isAngryTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}


void showSadExpressionTask(void* parameter) {
  isSadTaskRunning = true;
  drawOverlay(nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 77, 42);
  vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);

  int getSpottedDevice = seenDevices.size();
  spottedDevice = getSpottedDevice;
  showFindingCounter(targetConnects, susDevice, spottedDevice);  

  isSadTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}



void showFindingCounter(int sniffed, int susDevice, int spotted) {

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(5, 124);
  M5.Lcd.print("Spotted:");
  M5.Lcd.println(spotted);

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(100, 124);
  M5.Lcd.print("Sniffed:");
  M5.Lcd.println(sniffed);

  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(190, 124);
  M5.Lcd.print("Sus:");
  M5.Lcd.println(susDevice);
}