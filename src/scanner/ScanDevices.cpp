#include <NimBLEDevice.h>
#include <M5Cardputer.h>
#include <set>
#include <map>
#include <vector>
#include <algorithm>

#include "ScanDevices.h"
#include "../globals/globals.h"
#include "../helper/ManufacturerHelper.h"
#include "../helper/ServiceHelper.h"
#include "../helper/drawOverlay.h"
#include "../helper/showExpression.h"
#include "../logToSerialAndWeb/logger.h"
#include "../privacyCheck/devicePrivacy.h"


NimBLEScan* pScan = nullptr;
NimBLEClient *pClient = nullptr;

// Forward declarations of required services/classes
class SDLogger;
SDLogger sdLogger;


#define MAX_SEEN_DEVICES 1000

// vulnerableUUIDs
std::vector<std::string> vulnerableUUIDs = {
    "0000fd6f-0000-1000-8000-00805f9b34fb", // Apple Find My / AirTags (used for tracking)
    "0000feed-0000-1000-8000-00805f9b34fb", // Tile tracker
    "0000fef5-0000-1000-8000-00805f9b34fb", // Samsung SmartThings (also SmartTag)
    "0000fe95-0000-1000-8000-00805f9b34fb", // Xiaomi (Mi Band / sensors, sometimes leaks info)
    "0000fe2c-0000-1000-8000-00805f9b34fb", // Google Fast Pair (used to identify Android devices)
    "0000fe03-0000-1000-8000-00805f9b34fb", // Amazon Echo and Alexa devices
    "0000fe8f-0000-1000-8000-00805f9b34fb", // Facebook Beacon
    "0000fe2e-0000-1000-8000-00805f9b34fb", // Microsoft Swift Pair
    "00000020-5749-5448-0037-000000000000", // Withings / Nokia Health (seen in your logs )
    "932c32bd-0000-47a2-835a-a8d455b859dd", // Philips Hue BLE Setup
    "0000fff0-0000-1000-8000-00805f9b34fb"  // Unknown but commonly abused custom service (some malware)
};

bool isVulnerableService(const std::string& uuid) 
{
  return std::find(vulnerableUUIDs.begin(), vulnerableUUIDs.end(), uuid) != vulnerableUUIDs.end();
}

void scanForDevices() {
  NimBLEScan* pScan = NimBLEDevice::getScan();
  
  if (pScan == nullptr) {
    Serial.println("Scan instance creation failed.");
    return;  // Early exit if pScan is null
  }

  if (pScan != nullptr) {
    pScan->clearResults();        // 1. Clear previous results first
    pScan->setActiveScan(true);   // 2. Set active scan mode
    pScan->setInterval(45);       // 3. Set scan interval // old 1000
    pScan->setWindow(15);         // 4. Set scan window   // old 900
    delay(100);                   // Optional small delay for stability
  } else {
    Serial.println("⚠️ pScan is null!");
  }

  NimBLEScanResults results;

  if (pScan != nullptr) {
    results = pScan->getResults(3000);  // Scan 3 seconds to get scan results -> maybe check 3sec for smaller list and earlier new scan
  } else {
    Serial.println("⚠️ pScan is null! Cannot get scan results.");
  }
  
  if (results.getCount() == 0) {
     Serial.println("NO DEVICES FOUND");
  } else {
    logToSerialAndWeb("📲 DEVICES FOUND: " + String(results.getCount()));

    scanIsRunning = true;

    logToSerialAndWeb("📡 Scan Is Running");
  
    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice *device = results.getDevice(i);

      riskScore = 0;  // reset Risk Score for every seen device

      if (device != nullptr) {
          address = device->getAddress().toString().c_str();
          localName = device->haveName() ? String(device->getName().c_str()) : "< -- >";
          rssi = device->getRSSI();

          is_connectable = device->isConnectable();

          std::vector<uint8_t> payloadVec;
          handleDevicePrivacy(localName.c_str(), address.c_str(), spacedPayload.c_str(), payloadVec, is_connectable);

      } else {
          Serial.println("⚠️ device is null! Skipping.");
      }

      if (seenDevices.find(std::string(address.c_str())) != seenDevices.end()) {
        logToSerialAndWeb(String("🛑 Already seen: ") + address.c_str() + "\n");
        continue;
      } 

      allSpottedDevice++;

      if (is_connectable){
        logToSerialAndWeb("   Device is not connectable");

        // Risk Level
        String riskLevel = "🟢 Secure";
        if (riskScore >= 3 && riskScore < 6) riskLevel = "🟠 Moderate risk";
        else if (riskScore >= 6) riskLevel = "🔴 High risk";

        logToSerialAndWeb("📊 Risk Level: " + riskLevel + " Score: " + String(riskScore) + "\n");
        continue;
      }

      logToSerialAndWeb(String("   Trying to connect to MAC: ") + address);

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

      if (pClient != nullptr) {
          pClient->setConnectTimeout(5 * 1000); // Set 5s timeout
      } else {
          Serial.println("⚠️ pClient is null! Cannot set connect timeout.");
      }

      if (pClient != nullptr) {
        if (pClient->connect(*device) && !is_connectable) {
          if (pClient->discoverAttributes()) {
            logToSerialAndWeb("🔓 Connected and discovered attributes!");
            targetConnects++;
      
            if (!isGlassesTaskRunning && !isAngryTaskRunning) {
              xTaskCreate(showGlassesExpressionTask, "HappyFace", 2048, NULL, 0, NULL);
            }

            // Manufacturer handling
            if (device->haveManufacturerData()) {
              logToSerialAndWeb("   Manufacturer Data");
              std::string mfg = device->getManufacturerData();
              logToSerialAndWeb(String("     Manufacturer Data: ") + String(mfg.c_str()));
                  
              uint16_t manufacturerId = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
              String manufacturerName = getManufacturerName(manufacturerId);
              manuInfo = "     Manufacturer ID: 0x" + String(manufacturerId, HEX) + " (" + manufacturerName + ")";
              Serial.println(manuInfo);
            }
      
            deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);

            // Skipp Apple Products to speed up 
            if (deviceInfoService.indexOf("Apple Inc.") != -1) {
              //logToSerialAndWeb("🍏 APPLE DEVICE SKIPP");
              //logToSerialAndWeb("   ...");
              //continue;
            }

            batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
            heartRateService = HeartRateServiceHandler::readHeartRate(pClient);
            genericAccessService = GenericAccessServiceHandler::readGenericAccessInfo(pClient);
      
            bool isTarget = false;
            for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
              NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
              std::string serviceUuid = service->getUUID().toString();
              Serial.println(String("   Service UUID: ") + serviceUuid.c_str());

              // isVulnerableService
              bool vulnerable = isVulnerableService(serviceUuid.c_str());
              Serial.println(String("     isVulnerableService UUID: ") + serviceUuid.c_str() + " → " + (vulnerable ? "✅ YES" : "❌ NO"));
              if (vulnerable) riskScore += 2;
              // isVulnerableService

              uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

              for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
                NimBLERemoteCharacteristic* characteristic = *cIt;
                std::string charUuid = characteristic->getUUID().toString();
                Serial.println(String("     Characteristic UUID: ") + charUuid.c_str());
                uuidList.push_back("Characteristic UUID: " + std::string(charUuid.c_str()));
                
                if (characteristic) {
                  std::string name = characteristic->readValue();

                  if (!name.empty()) {
                    Serial.println(String("     Characteristic Name: ") + name.c_str());
                    nameList.push_back(std::string(name.c_str()));
                  }
                } else {
                  Serial.println("   Device Name Characteristic not found.");
                }
              }

              if (device != nullptr && !device->getName().empty()) {
                localName = device->getName().c_str();
                //logToSerialAndWeb("Replace localName with connected device->getName");
              }

              if (isTargetDevice(localName.c_str(), address.c_str(), serviceUuid.c_str(), deviceInfoService.c_str())) {
                targetFound = true;
                susDevice++;
                logToSerialAndWeb("Target Message: !!! Target detected !!!");
                delay(2000);
                if (!isAngryTaskRunning) {
                  //logToSerialAndWeb("showAngryExpressionTask");
                  xTaskCreate(showAngryExpressionTask, "AngryFace", 2048, NULL, 4, NULL);
                }
                isTarget = true;
                break;
              }
            }

            logToSerialAndWeb("📝 Device Infos");
            logToSerialAndWeb(String("   Adress: " + address));
            logToSerialAndWeb(String("   Name: " + localName));
            delay(100);
            logToSerialAndWeb("   Device Name: ");
            for (const auto& names : nameList) {
              if (!names.empty()) {
                logToSerialAndWeb(String("     - ") + names.c_str());
              }
            }
            
            delay(100);
            float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
            logToSerialAndWeb("🛰️ Distance: " + String(distance, 2) + " m");
            delay(100);
            logToSerialAndWeb("     - RSSI: " + String(rssi));
            delay(100);

            // Risk Level
            String riskLevel = "🟢 Secure";
            if (riskScore >= 3 && riskScore < 6) riskLevel = "🟠 Moderate risk";
            else if (riskScore >= 6) riskLevel = "🔴 High risk";

            logToSerialAndWeb("📊 Risk Level: " + riskLevel + " Score: " + String(riskScore) + "\n");

            String riskLevelSd = "Secure";
            if (riskScore >= 3 && riskScore < 6) riskLevelSd = "Moderate risk";
            else if (riskScore >= 6) riskLevelSd = "High risk";
            String riskLevelSdCard = "Risk Level: " + riskLevelSd + " Score: " + String(riskScore);

            delay(100);

            logToSerialAndWeb("----------------------------------\n");
        
            // Move to isTargetDevice to log on SD card
            sdLogger.writeDeviceInfo(address, localName, riskLevelSdCard, nameList, manuInfo, uuidList, deviceInfoService, batteryLevelService, genericAccessService);
            //logToSerialAndWeb("Write Data to SD Logger");

            // Clear uuidList / nameList / riskScore after Stored to SD Card
            uuidList.clear();
            nameList.clear();
          }
        } else {
          logToSerialAndWeb("🔒 Attribute discovery failed: " + address);

          // Risk Level
          String riskLevel = "🟢 Secure";
          if (riskScore >= 3 && riskScore < 6) riskLevel = "🟠 Moderate risk";
          else if (riskScore >= 6) riskLevel = "🔴 High risk";

          logToSerialAndWeb("📊 Risk Level: " + riskLevel + " Score: " + String(riskScore) + "\n");

          if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
            //logToSerialAndWeb("showSadExpressionTask");
            xTaskCreate(showSadExpressionTask, "SadFace", 2048, NULL, 1, NULL);
          }
        }
      }
      if (pClient != nullptr && pClient->isConnected()) {
        pClient->disconnect();
      }
      NimBLEDevice::deleteClient(pClient);
      pClient = nullptr;
    }
    delay(100);
    logToSerialAndWeb("##########################");
    delay(100);
    logToSerialAndWeb("📊 Scan Summary:");
    delay(100);
    logToSerialAndWeb("Sniffed:    " + String(targetConnects));
    delay(100);
    logToSerialAndWeb("Leaked:     " + String(leakedCounter));
    delay(100);
    logToSerialAndWeb("Suspicious: " + String(susDevice));
    delay(100);
    
    logToSerialAndWeb("##########################\n");
    delay(100);
    scanIsRunning = false;
  }
}
