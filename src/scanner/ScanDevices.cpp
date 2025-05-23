#include <NimBLEDevice.h>
#include <M5Cardputer.h>
#include <set>

#include "ScanDevices.h"
#include "../globals/globals.h"
#include "../helper/ManufacturerHelper.h"
#include "../helper/ServiceHelper.h"
#include "../helper/drawOverlay.h"
#include "../helper/showExpression.h"
#include "../logToSerialAndWeb/logger.h"


NimBLEScan* pScan = nullptr;
NimBLEClient *pClient = nullptr;

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

  pScan->clearResults();        // 1. Clear previous results first
  pScan->setActiveScan(true);   // 2. Set active scan mode
  pScan->setInterval(1000);     // 3. Set scan interval
  pScan->setWindow(900);        // 4. Set scan window
  delay(100);                   // (Optional small delay for stability)
  
  NimBLEScanResults results = pScan->getResults(5 * 1000);  // Scan 5 seconds to get scan results -> maybe check 3sec for smaller list and earlier new scan
  if (results.getCount() == 0) {
     logToSerialAndWeb("NO DEVICES FOUND");
  } else {
    logToSerialAndWeb("Devices found: " + String(results.getCount()));

    scanIsRunning = true;
    logToSerialAndWeb("Scan Is Running");
  
    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice *device = results.getDevice(i);

      address = device->getAddress().toString().c_str();
      localName = device->haveName() ? String(device->getName().c_str()) : "Unknown";
      int rssi = device->getRSSI();

      if (seenDevices.empty()) {
        logToSerialAndWeb("  seenDevices is currently empty");
      }
      logToSerialAndWeb("  seenDevices size: " + String(seenDevices.size() + 1));
      logToSerialAndWeb(String("  Trying to access address: ") + address);

      try {
        if (seenDevices.find(std::string(address.c_str())) != seenDevices.end()) {
          logToSerialAndWeb(String("🛑 Already seen: ") + address.c_str());
          continue;
        } else {
          allSpottedDevice++;
        }
      } catch (...) {
        logToSerialAndWeb("Exception caught accessing seenDevices");
      }
      
      logToSerialAndWeb("  Trying to connect for service discovery...");

      logToSerialAndWeb("   connecting for service discovery...");
      pClient = NimBLEDevice::createClient();
      delay(1000);

      if (!pClient) { // Make sure the client was created
        break;
      } else {
        //logToSerialAndWeb("INSERT SEEN DEVICE AT PCLIENT");
        seenDevices.insert(std::string(address.c_str()));
      }

      if (seenDevices.size() >= MAX_SEEN_DEVICES) {
        seenDevices.clear();
      }

      pClient->setConnectTimeout(5 * 1000); // 5sec TimeOut -> default 30sec

      if (pClient->connect(*device)) {
        if (pClient->discoverAttributes()) {
          logToSerialAndWeb("✅ Connected and discovered attributes!");
          targetConnects++;
    
          if (!isGlassesTaskRunning && !isAngryTaskRunning) {
            xTaskCreate(showGlassesExpressionTask, "HappyFace", 2048, NULL, 0, NULL);
          }

          // Manufacturer handling
          if (device->haveManufacturerData()) {
            logToSerialAndWeb("Manufacturer Data");
            std::string mfg = device->getManufacturerData();
            logToSerialAndWeb(String("  Manufacturer Data: ") + String(mfg.c_str()));
                
            uint16_t manufacturerId = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
            String manufacturerName = getManufacturerName(manufacturerId);
            manuInfo = "  Manufacturer ID: 0x" + String(manufacturerId, HEX) + " (" + manufacturerName + ")";
            Serial.println(manuInfo);
          }
    
          deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);

          // Skipp Apple Products to speed up 
          if (deviceInfoService.indexOf("Apple Inc.") != -1) {
            //logToSerialAndWeb("APPLE DEVICE SKIPP");
            //continue;
          }

          batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
          heartRateService = HeartRateServiceHandler::readHeartRate(pClient);
          genericAccessService = GenericAccessServiceHandler::readGenericAccessInfo(pClient);
    
          bool isTarget = false;
          for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
            NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
            std::string serviceUuid = service->getUUID().toString();
            logToSerialAndWeb(String("Service UUID: ") + serviceUuid.c_str());

            uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

            for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
              NimBLERemoteCharacteristic* characteristic = *cIt;
              std::string charUuid = characteristic->getUUID().toString();
              logToSerialAndWeb(String("  Characteristic UUID: ") + charUuid.c_str());
              uuidList.push_back("  Characteristic UUID: " + std::string(charUuid.c_str()));
              
              if (characteristic) {
                std::string name = characteristic->readValue();

                if (!name.empty()) {
                  //logToSerialAndWeb(String("Gerätename: " + name.c_str()));
                  nameList.push_back(std::string(name.c_str()));
                }
              } else {
                logToSerialAndWeb("Device Name Characteristic not found.");
              }
            }

            if (!device->getName().empty()) {
              localName = device->getName().c_str();
              //logToSerialAndWeb("Replace localName with connected device->getName");
            }

            if (isTargetDevice(localName.c_str(), address.c_str(), serviceUuid.c_str(), deviceInfoService.c_str())) {
              targetFound = true;
              susDevice++;
              Serial.println("Target Message: !!! Target detected !!!");
              delay(2000);
              if (!isAngryTaskRunning) {
                logToSerialAndWeb("showAngryExpressionTask");
                xTaskCreate(showAngryExpressionTask, "AngryFace", 2048, NULL, 4, NULL);
                //xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 4, NULL);
              }
              isTarget = true;
              break;
            }
          }
      
          logToSerialAndWeb("Device Infos");
          logToSerialAndWeb(String("  Adress: " + address));
          logToSerialAndWeb(String("  Local Name: " + localName));
          logToSerialAndWeb("  Device Name: ");
          for (const auto& names : nameList) {
            if (!names.empty()) {
              logToSerialAndWeb(String("    - ") + names.c_str());
            }
          }

          logToSerialAndWeb("  RSSI: " + rssi);
      
          float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
          logToSerialAndWeb("  Distance: " + String(distance, 2) + " m");

          logToSerialAndWeb("----------------------------------\n");
      
          // Move to isTargetDevice to log on SD card
          sdLogger.writeDeviceInfo(address, localName, nameList, manuInfo, uuidList, deviceInfoService, batteryLevelService, genericAccessService);
          //logToSerialAndWeb("Write Data to SD Logger");

          // Clear uuidList after Stored to SD Card
          uuidList.clear();
          nameList.clear();
        } else {
          logToSerialAndWeb("Attribute discovery failed.");
        }
      } else {
        Serial.println("Attribute discovery FAILED.");
        if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
          logToSerialAndWeb("showSadExpressionTask");
          xTaskCreate(showSadExpressionTask, "SadFace", 2048, NULL, 1, NULL);
        }
      }
      if (pClient->isConnected()) {
        pClient->disconnect();
      }
      NimBLEDevice::deleteClient(pClient);
      pClient = nullptr;
    }
    logToSerialAndWeb("###############################\n");
    scanIsRunning = false;
  }
}
