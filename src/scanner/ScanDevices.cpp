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

bool isTarget = false;

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

struct IBeaconInfo {
  bool valid = false;
  std::string uuid;
  uint16_t major = 0;
  uint16_t minor = 0;
  int8_t txPower = 0;
};

struct DeviceAssessment {
  bool hackable = false;

  String behavior;      // Connectable / Broadcast only / Locked
  String protocol;      // Standard BLE / Proprietary
  String control;       // Accessible / Not accessible
  String explanation;   // Human explanation
};

IBeaconInfo parseIBeacon(const std::string& mfg) {
  IBeaconInfo info;

  if (mfg.size() < 25) return info;

  const uint8_t* d = (const uint8_t*)mfg.data();

  // Apple Company ID + iBeacon prefix
  if (d[0] != 0x4C || d[1] != 0x00) return info;
  if (d[2] != 0x02 || d[3] != 0x15) return info;

  char uuidStr[37];
  snprintf(uuidStr, sizeof(uuidStr),
    "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    d[4], d[5], d[6], d[7],
    d[8], d[9],
    d[10], d[11],
    d[12], d[13],
    d[14], d[15], d[16], d[17], d[18], d[19]
  );

  info.uuid = uuidStr;
  info.major = (d[20] << 8) | d[21];
  info.minor = (d[22] << 8) | d[23];
  info.txPower = (int8_t)d[24];
  info.valid = true;

  return info;
}

float estimateDistance(int txPower, int rssi) {
  if (rssi == 0) return -1.0;
  float ratio = rssi * 1.0 / txPower;
  if (ratio < 1.0) return pow(ratio, 10);
  return 0.89976 * pow(ratio, 7.7095) + 0.111;
}

DeviceAssessment assessDevice(bool connectable, bool hasWritableChar, bool encrypted,  bool proprietary)
{
  DeviceAssessment d;

  if (!connectable) {
    d.hackable = false;
    d.behavior = "Broadcast only";
    d.protocol = "Standard BLE";
    d.control  = "❌ Not accessible via BLE";
    d.explanation =
      "This device only broadcasts information and cannot be connected to.";
  }
  else if (encrypted) {
    d.hackable = false;
    d.behavior = "Connectable but locked";
    d.protocol = proprietary ? "Proprietary" : "Standard BLE";
    d.control  = "❌ Not accessible via BLE";
    d.explanation =
      "A secure pairing is required. Only authorized apps can control it.";
  }
  else if (proprietary) {
    d.hackable = false;
    d.behavior = "Connectable";
    d.protocol = "Proprietary";
    d.control  = "❌ Not accessible via BLE";
    d.explanation =
      "The communication protocol is unknown and not publicly documented.";
  }
  else if (hasWritableChar) {
    d.hackable = true;
    d.behavior = "Connectable";
    d.protocol = "Standard BLE";
    d.control  = "✅ Accessible via BLE";
    d.explanation =
      "Commands can potentially be sent without authentication.";
  }
  else {
    d.hackable = false;
    d.behavior = "Connectable";
    d.protocol = "Standard BLE";
    d.control  = "❌ No controllable characteristics found";
    d.explanation =
      "The device exposes no writable BLE characteristics.";
  }

  return d;
}

void startBleScan() {
  Serial.println("▶️ Starting BLE scan...");
  scanIsRunning = false;   // allow scanForDevices() to trigger
}

void stopBleScan() {
  Serial.println("🛑 Stopping BLE scan...");
  NimBLEDevice::getScan()->stop();
  scanIsRunning = false;
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

      // New risk scoring system
      bool hasVulnerableUUID = false;
      bool hasCustomService = false;
      bool hasSensitiveCharacteristic = false;
      bool hasWeakName = false;
      bool isUnknownManufacturer = false;
      bool isSecurityOrTrackingDevice = false;
      bool noAuthOrEncryption = false;
      bool hasWritableChar = false;
      bool encrypted = false;
      bool proprietary = false;

      bool isIBeacon = false;
      IBeaconInfo beacon;

      if (device != nullptr) {
        address = device->getAddress().toString().c_str();
        localName = device->haveName() ? String(device->getName().c_str()) : "< -- >";
        rssi = device->getRSSI();
        is_connectable = device->isConnectable();

        // Dedupe: Insert into seenDevices immediately (before any connect attempt)
        if (seenDevices.find(std::string(address.c_str())) != seenDevices.end()) {
          logToSerialAndWeb(String("🛑 Already seen: ") + address.c_str() + "\n");
          continue;
        }
        seenDevices.insert(std::string(address.c_str()));

        std::vector<uint8_t> payloadVec;
        handleDevicePrivacy(localName.c_str(), address.c_str(), spacedPayload.c_str(), payloadVec, is_connectable);

        // Risk factor: Weak/default device name
        if (localName == "< -- >" || localName == "BLE Device" || localName == "Unknown") {
          hasWeakName = true;
        }

        // Risk factor: Device type (simple heuristic)
        if (localName.indexOf("Tracker") != -1 || localName.indexOf("Tag") != -1 || localName.indexOf("Medical") != -1 || localName.indexOf("Security") != -1) {
          isSecurityOrTrackingDevice = true;
        }

        if (device->haveManufacturerData()) {
          std::string mfg = device->getManufacturerData();
          uint16_t manufacturerId = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
          String manufacturerName = getManufacturerName(manufacturerId);

          // Detect iBeacon
          beacon = parseIBeacon(mfg);
          if (beacon.valid) {
            isIBeacon = true;
            logToSerialAndWeb("🟦 iBeacon detected!");
            logToSerialAndWeb("   UUID:  " + String(beacon.uuid.c_str()));
            logToSerialAndWeb("   Major: " + String(beacon.major));
            logToSerialAndWeb("   Minor: " + String(beacon.minor));
            logToSerialAndWeb("   TX:    " + String(beacon.txPower));
          }

          if (manufacturerName == "Unknown Manufacturer") {
            isUnknownManufacturer = true;
          }
        }

        // Risk factor: Service UUID (advertised)
        std::string serviceUuid = device->getServiceUUID().toString();
        if (!serviceUuid.empty()) {
          if (isVulnerableService(serviceUuid)) {
            hasVulnerableUUID = true;
          }
          // Custom/private service UUID (not standard 16-bit)
          if (serviceUuid.length() > 8 && serviceUuid.find("0000") != 0) {
            hasCustomService = true;
          }

          if (hasCustomService) {
              proprietary = true;
          }
        }

        // Risk factor: Sensitive characteristics (simple heuristic)
        // This requires connection, so only check if connectable
        if (is_connectable) {
          // ...existing code for connecting and discovering attributes...
          // After discovering characteristics, check for sensitive ones
          // Example: device info, health, location
          // You can add more checks here as needed
        }

        // Risk factor: No authentication/encryption (placeholder)
        // If you can check BLE security level, add +2 for no auth/encryption

        // ...existing code for target device check...
      } else {
        Serial.println("⚠️ device is null! Skipping.");
        continue;
      }

      isTarget = false;
      allSpottedDevice++;

      // Print device connectability
      if (!is_connectable) {
        logToSerialAndWeb("   Device is not connectable");
      }

      logToSerialAndWeb(String("   Trying to connect to MAC: ") + address);

      pClient = NimBLEDevice::createClient();
      delay(1000);

      if (!pClient) { // Make sure the client was created
        break;
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
        //if (pClient->connect(*device) && !is_connectable) {
        if (is_connectable && pClient->connect(*device)) {  
          if (pClient->discoverAttributes()) {

            deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);

            // Skipp Apple Products to speed up 
            if (deviceInfoService.indexOf("Apple Inc.") != -1) {
              logToSerialAndWeb("🍏 APPLE DEVICE SKIPP");
              logToSerialAndWeb("   ...");
              continue;
            }

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
                
              if (mfg.size() >= 2) {
                uint16_t manufacturerId = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
                String manufacturerName = getManufacturerName(manufacturerId);
                manuInfo = "     Manufacturer ID: 0x" + String(manufacturerId, HEX) + " (" + manufacturerName + ")";
                Serial.println(manuInfo);
              }
            }
      
            batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
            heartRateService = HeartRateServiceHandler::readHeartRate(pClient);
            temperatureService = TemperatureServiceHandler::readTemperature(pClient);
            //genericAccessService = GenericAccessServiceHandler::readGenericAccessInfo(pClient);
      
            for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
              NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
              std::string serviceUuid = service->getUUID().toString();
              Serial.println(String("   Service UUID: ") + serviceUuid.c_str());

              // isVulnerableService
              bool vulnerable = isVulnerableService(serviceUuid.c_str());
              Serial.println(String("     isVulnerableService UUID: ") + serviceUuid.c_str() + " → " + (vulnerable ? "✅ YES" : "❌ NO"));

              uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

              for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
                NimBLERemoteCharacteristic* characteristic = *cIt;
                std::string charUuid = characteristic->getUUID().toString();
                Serial.println(String("     Characteristic UUID: ") + charUuid.c_str());
                uuidList.push_back("Characteristic UUID: " + std::string(charUuid.c_str()));

                if (characteristic->canWrite() || characteristic->canWriteNoResponse()) {
                    hasWritableChar = true;
                }
                                
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

            DeviceAssessment assessment = assessDevice(is_connectable, hasWritableChar, encrypted, proprietary);
            logToSerialAndWeb("");
            logToSerialAndWeb("⏳ Device Assessment");
            logToSerialAndWeb("   - Behavior: " + assessment.behavior);
            logToSerialAndWeb("   - Protocol: " + assessment.protocol);
            logToSerialAndWeb("   - Control: " + assessment.control);
            logToSerialAndWeb(
            
            String("🥷🏻 Hackable via BLE: ") + (assessment.hackable ? "✅ YES" : "❌ NO"));
            logToSerialAndWeb("");
            logToSerialAndWeb("What this means:");
            logToSerialAndWeb(assessment.explanation);
            logToSerialAndWeb("");

            // iBeacon info
            if (isIBeacon) {
              logToSerialAndWeb("👁️ Beacon Type: iBeacon");
              logToSerialAndWeb("   UUID:  " + String(beacon.uuid.c_str()));
              logToSerialAndWeb("   Major: " + String(beacon.major));
              logToSerialAndWeb("   Minor: " + String(beacon.minor));
              float beaconDistance = estimateDistance(beacon.txPower, rssi);
              logToSerialAndWeb("📏 Beacon Distance: ~" + String(beaconDistance, 2) + " m");

              sdLogger.writeIBeaconInfo(String(beacon.uuid.c_str()), String(beacon.major), String(beacon.minor), String(beaconDistance, 2));
            }

            delay(100);

            logToSerialAndWeb("----------------------------------\n");
        
            // Move to isTargetDevice to log on SD card
            sdLogger.writeDeviceInfo(address, localName, nameList, manuInfo, uuidList, deviceInfoService, batteryLevelService, genericAccessService);
            
            //logToSerialAndWeb("Write Data to SD Logger");

            // Clear uuidList / nameList after Stored to SD Card
            uuidList.clear();
            nameList.clear();
          }
        } else {
          logToSerialAndWeb("🔒 Attribute discovery failed: " + address);
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
    logToSerialAndWeb("Spotted:    " + String(allSpottedDevice));
    delay(100);
    logToSerialAndWeb("Suspicious: " + String(susDevice));
    delay(100);
    
    logToSerialAndWeb("##########################\n");
    delay(100);

    xTaskCreate(showHappyExpressionTask, "HappyFace", 2048, NULL, 1, NULL);

    scanIsRunning = false;
  }
}
