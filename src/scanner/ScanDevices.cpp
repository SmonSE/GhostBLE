#include <NimBLEDevice.h>
#include <M5Cardputer.h>
#include <set>

#include "ScanDevices.h"
#include "../globals/globals.h"
#include "../helper/ManufacturerHelper.h"
#include "../helper/ServiceHelper.h"
#include "../helper/drawOverlay.h"
#include "../helper/showExpression.h"


NimBLEUUID serviceUuid("ABCD");

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

  pScan->setActiveScan(true);  // Active scan
  pScan->setInterval(1000);    // Adjust interval to 1 second
  pScan->setWindow(900);       // Adjust window to 900ms (90% of interval)
  pScan->clearResults();       // Clear previous scan results
  
  NimBLEScanResults results = pScan->getResults(10 * 1000);  // Scan 10 seconds to get scan results
  if (results.getCount() == 0) {
    // Serial.println(EMPTY COUNT);
  } else {
    scanIsRunning = true;
    Serial.println("Scan Is Running");

    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice *device = results.getDevice(i);

      String address = device->getAddress().toString().c_str();
      String localName = device->haveName() ? String(device->getName().c_str()) : "Unknown";
      int rssi = device->getRSSI();

      if (seenDevices.empty()) {
        Serial.println("🧪 seenDevices is currently empty");
      }
      allSpottedDevice++;
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

      Serial.println("   connecting for service discovery...");
      NimBLEClient *pClient = NimBLEDevice::createClient();
      delay(1000);

      if (!pClient) { // Make sure the client was created
        break;
      }

      if (pClient->connect(*device)) {
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
      } else {
        Serial.println("Attribute discovery FAILED.");
        if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
          Serial.println("showSadExpressionTask");
          xTaskCreate(showSadExpressionTask, "SadFace", 2048, NULL, 1, NULL);
        }
      }
      NimBLEDevice::deleteClient(pClient);
    }
    Serial.println("###############################\n");
    scanIsRunning = false;
  }
}
