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


// PRIVACY NEW
// Ganz oben hinzufügen
#include <map>
#include <vector>
#include <algorithm>

// Global oder static in der Datei:
struct DeviceInfo {
  std::string last_mac;
  uint8_t mac_change_count = 0;
  std::vector<std::string> seen_macs;
};

std::map<std::string, DeviceInfo> mac_history;

bool isASCII(const std::string& s) {
  return std::all_of(s.begin(), s.end(), [](char c){ return c >= 32 && c <= 126; });
}

void handleDevicePrivacy(const std::string& name, const std::string& mac, const std::string& adv_data) {
  auto& info = mac_history[name];

  if (!info.seen_macs.empty() && std::find(info.seen_macs.begin(), info.seen_macs.end(), mac) == info.seen_macs.end()) {
    info.mac_change_count++;
    info.seen_macs.push_back(mac);
  } else if (info.seen_macs.empty()) {
    info.seen_macs.push_back(mac);
  }

  bool is_static_mac = info.mac_change_count == 0;
  bool adv_contains_cleartext = adv_data.find("http") != std::string::npos || isASCII(adv_data);

  Serial.printf("🔍 %s MAC: %s | Rotating: %s | Cleartext: %s\n",
                name.c_str(), mac.c_str(),
                is_static_mac ? "❌" : "✅",
                adv_contains_cleartext ? "❗" : "✅");
}

// Helper function to convert raw payload bytes to hex string
String payloadToHexString(const String& payload) {
    String hexStr = "";
    for (size_t i = 0; i < payload.length(); i++) {
        uint8_t b = payload.charAt(i);
        if (b < 0x10) hexStr += "0";
        hexStr += String(b, HEX);
        hexStr += " ";
    }
    return hexStr;
}
// PRIVACY NEW


void scanForDevices() {
  NimBLEScan* pScan = NimBLEDevice::getScan();
  
  if (pScan == nullptr) {
    Serial.println("Scan instance creation failed.");
    return;  // Early exit if pScan is null
  }

  if (pScan != nullptr) {
    pScan->clearResults();        // 1. Clear previous results first
    pScan->setActiveScan(false);  // 2. Set active scan mode
    pScan->setInterval(1000);     // 3. Set scan interval
    pScan->setWindow(900);        // 4. Set scan window
    delay(100);                   // Optional small delay for stability
  } else {
    logToSerialAndWeb("⚠️ pScan is null!");
  }

  NimBLEScanResults results;

  if (pScan != nullptr) {
    results = pScan->getResults(3000);  // Scan 3 seconds to get scan results -> maybe check 3sec for smaller list and earlier new scan
  } else {
    logToSerialAndWeb("⚠️ pScan is null! Cannot get scan results.");
  }
  
  if (results.getCount() == 0) {
     logToSerialAndWeb("NO DEVICES FOUND");
  } else {
    logToSerialAndWeb("📲 DEVICES FOUND: " + String(results.getCount()));

    scanIsRunning = true;
    logToSerialAndWeb("📡 Scan Is Running");
  
    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice *device = results.getDevice(i);

      if (device != nullptr) {
          address = device->getAddress().toString().c_str();
          localName = device->haveName() ? String(device->getName().c_str()) : "<NoName>";
          rssi = device->getRSSI();

          std::vector<uint8_t> payloadVec = device->getPayload();
          String payload = "";
          for (size_t i = 0; i < payloadVec.size(); i++) {
              if (payloadVec[i] < 16) payload += "0";
              payload += String(payloadVec[i], HEX);
          }
          payload.toUpperCase();
      } else {
          logToSerialAndWeb("⚠️ device is null! Skipping.");
      }

      if (seenDevices.empty()) {
        logToSerialAndWeb("   seenDevices is currently empty");
      }

      // ➕ Rufe Privacy-Analyse auf
      handleDevicePrivacy(localName.c_str(), address.c_str(), payload.c_str());

      logToSerialAndWeb(String("   Trying to connect to address: ") + address);

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
          logToSerialAndWeb("⚠️ pClient is null! Cannot set connect timeout.");
      }

      if (pClient != nullptr) {
        if (pClient->connect(*device)) {
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
              logToSerialAndWeb("🍏 APPLE DEVICE SKIPP");
              logToSerialAndWeb("   ...");
              continue;
            }

            batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
            heartRateService = HeartRateServiceHandler::readHeartRate(pClient);
            //genericAccessService = GenericAccessServiceHandler::readGenericAccessInfo(pClient);
      
            bool isTarget = false;
            for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
              NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
              std::string serviceUuid = service->getUUID().toString();
              logToSerialAndWeb(String("   Service UUID: ") + serviceUuid.c_str());

              uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

              for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
                NimBLERemoteCharacteristic* characteristic = *cIt;
                std::string charUuid = characteristic->getUUID().toString();
                logToSerialAndWeb(String("     Characteristic UUID: ") + charUuid.c_str());
                uuidList.push_back("Characteristic UUID: " + std::string(charUuid.c_str()));
                
                if (characteristic) {
                  std::string name = characteristic->readValue();

                  if (!name.empty()) {
                    //logToSerialAndWeb(String("Gerätename: " + name.c_str()));
                    nameList.push_back(std::string(name.c_str()));
                  }
                } else {
                  logToSerialAndWeb("   Device Name Characteristic not found.");
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
                  logToSerialAndWeb("showAngryExpressionTask");
                  xTaskCreate(showAngryExpressionTask, "AngryFace", 2048, NULL, 4, NULL);
                  //xTaskCreate(showThugLifeExpressionTask, "ThugLifeFace", 2048, NULL, 4, NULL);
                }
                isTarget = true;
                break;
              }
            }

            hexPayload = payloadToHexString(payload);
        
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
            logToSerialAndWeb("Payload (hex): " + hexPayload);
            delay(100);
            logToSerialAndWeb("----------------------------------\n");
        
            // Move to isTargetDevice to log on SD card
            sdLogger.writeDeviceInfo(address, localName, nameList, manuInfo, uuidList, deviceInfoService, batteryLevelService, genericAccessService);
            //logToSerialAndWeb("Write Data to SD Logger");

            // Clear uuidList after Stored to SD Card
            uuidList.clear();
            nameList.clear();
          } 
        } else {
          logToSerialAndWeb("🔒 Attribute discovery failed: " + address);
          if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
            logToSerialAndWeb("showSadExpressionTask");
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
    logToSerialAndWeb("Spotted:    " + String(allSpottedDevice));
    delay(100);
    logToSerialAndWeb("Sniffed:    " + String(targetConnects));
    delay(100);
    logToSerialAndWeb("Suspicious: " + String(susDevice));
    delay(100);
    logToSerialAndWeb("##########################\n");
    delay(100);
    scanIsRunning = false;
  }
}
