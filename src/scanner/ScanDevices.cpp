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
#include "../analyzer/ExposureAnalyzer.h"
#include "../models/DeviceInfo.h"
#include "../privacyCheck/ExposureClassifier.h"


NimBLEScan* pScan = nullptr;
NimBLEClient *pClient = nullptr;

// Forward declarations of required services/classes
class SDLogger;
SDLogger sdLogger;

bool isTarget = false;
std::string bestDeviceName = "";

#define MAX_SEEN_DEVICES 1000

struct IBeaconInfo {
  bool valid = false;
  std::string uuid;
  uint16_t major = 0;
  uint16_t minor = 0;
  int8_t txPower = 0;
};

static const std::vector<std::string> roomWords = {
    "wohnzimmer", "küche", "kueche", "bad",
    "schlafzimmer", "office", "living",
    "bedroom", "kitchen", "bath"
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

void startBleScan() {
  Serial.println("▶️ Starting BLE scan...");
  scanIsRunning = false;   // allow scanForDevices() to trigger
}

void stopBleScan() {
  Serial.println("🛑 Stopping BLE scan...");
  NimBLEDevice::getScan()->stop();
  scanIsRunning = false;
}

bool isPrintableText(const std::string& s)
{
    if (s.empty()) return false;

    for (char c : s)
    {
        // allow printable ASCII only
        if (c < 32 || c > 126)
            return false;
    }
    return true;
}

String bytesToHexString(const std::string& data)
{
    String out;

    for (uint8_t b : data)
    {
        if (b < 0x10) out += "0";
        out += String(b, HEX);
        out += " ";
    }

    out.toUpperCase();
    return out;
}

static const char* tierToString(ExposureTier tier)
{
    switch(tier)
    {
        case ExposureTier::Passive: return "PASSIVE";
        case ExposureTier::Active:  return "ACTIVE";
        case ExposureTier::Consent: return "CONSENT";
        default: return "NONE";
    }
}

void scanForDevices() {

  DeviceInfo dev;
  uint16_t manufacturerId = 0;
  String manufacturerName = "Unknown";

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
        localName = device->haveName() ? String(device->getName().c_str()) : "";
        rssi = device->getRSSI();
        is_connectable = device->isConnectable();

        // Dedupe: Insert into seenDevices immediately (before any connect attempt)
        if (seenDevices.find(std::string(address.c_str())) != seenDevices.end()) {
          logToSerialAndWeb(String("🛑 Already seen: ") + address.c_str() + "\n");
          continue;
        }
        seenDevices.insert(std::string(address.c_str()));

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
          manufacturerId = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
          manufacturerName = getManufacturerName(manufacturerId);

          // Detect iBeacon
          beacon = parseIBeacon(mfg);
          if (beacon.valid) {
            isIBeacon = true;
            logToSerialAndWeb("iBeacon detected!");
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
          if (serviceUuid.length() > 8 && serviceUuid.find("0000") != 0) {
            hasCustomService = true;
          }

          if (hasCustomService) {
              proprietary = true;
          }
        }
      } else {
        Serial.println("⚠️ device is null! Skipping.");
        continue;
      }

      // --- Payload direkt holen ---
      std::vector<uint8_t> payloadVec = device->getPayload();

      // --- daraus String erzeugen ---
      std::string advData(payloadVec.begin(), payloadVec.end());

      handleDevicePrivacy(
          std::string(localName.c_str()),
          std::string(address.c_str()),
          advData,
          payloadVec,
          is_connectable,
          dev
      );

      isTarget = false;
      allSpottedDevice++;

      // Print device connectability
      if (!is_connectable) {
        logToSerialAndWeb("   Device is not connectable");
      }

      //logToSerialAndWeb(String("   Trying to connect to MAC: ") + address);

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

            if (device->haveName()) {
                dev.advHasName = true;
            }

            deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);

            if (!deviceInfoService.isEmpty()) {
                dev.gattHasName = true;
            }

            bestDeviceName = std::string(deviceInfoService.c_str());

            logToSerialAndWeb("🔓 Connected and discovered attributes!");
            targetConnects++;

            if (!isGlassesTaskRunning && !isAngryTaskRunning) {
              xTaskCreate(showGlassesExpressionTask, "BLEGlases", 4096, NULL, 0, NULL);
            }
      
            //batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
            //heartRateService = HeartRateServiceHandler::readHeartRate(pClient);
            //temperatureService = TemperatureServiceHandler::readTemperature(pClient);
            //genericAccessService = GenericAccessServiceHandler::readGenericAccessInfo(pClient);
      
            for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
              NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
              std::string serviceUuid = service->getUUID().toString();
              uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

              for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
                NimBLERemoteCharacteristic* characteristic = *cIt;
                std::string charUuid = characteristic->getUUID().toString();
                //Serial.println(String("     Characteristic UUID: ") + charUuid.c_str());
                uuidList.push_back("Characteristic UUID: " + std::string(charUuid.c_str()));

                if (charUuid == "2a24") {               // Model Number
                    dev.gattHasModelInfo = false;
                }

                if (charUuid == "2a29" ||               // Manufacturer Name
                    charUuid == "2a25") {               // Serial Number
                    dev.gattHasIdentityInfo = false;
                }

                if (characteristic->canWrite() || characteristic->canWriteNoResponse()) {
                    hasWritableChar = true;
                }
                                
                if (characteristic)
                {
                    std::string rawValue = characteristic->readValue();

                    if (!rawValue.empty())
                    {
                        if (isPrintableText(rawValue))
                        {
                            //Serial.println(String("     Characteristic Name: ") + rawValue.c_str());
                            dev.gattHasName = true;   // ← missing in many cases
                            nameList.push_back(rawValue);

                            if (looksLikePersonalName(rawValue))
                            {
                                dev.gattHasPersonalName = true;
                            }

                            if (looksLikeIdentityData(rawValue))
                                dev.gattHasIdentityInfo = true;

                            if (looksLikeEnvironmentName(rawValue))
                                dev.gattHasEnvironmentName = true;

                            if (bestDeviceName.empty())
                            {
                                bestDeviceName = rawValue;
                            }
                        }
                    }
                }
                else
                {
                    Serial.println("   Device Name Characteristic not found.");
                }
              }

              if (device != nullptr && !device->getName().empty()) {
                localName = device->getName().c_str();
                //logToSerialAndWeb("Replace localName with connected device->getName");
                //dev.gattHasName = true;
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

            logToSerialAndWeb("Device Infos");
            logToSerialAndWeb(String("   Adress: " + address));
            //logToSerialAndWeb(String("   Name: " + String(bestDeviceName.c_str())));
            delay(100);
            logToSerialAndWeb("   Device Name: ");
            for (const auto& names : nameList) {
              if (!names.empty()) {
                logToSerialAndWeb(String("     - ") + names.c_str());
              }
            }
            
            delay(100);
            float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
            logToSerialAndWeb("Distance: " + String(distance, 2) + " m");
            delay(100);
            logToSerialAndWeb("     - RSSI: " + String(rssi));
            delay(100);

            // iBeacon info
            if (isIBeacon) {
              beaconsFound++;
              logToSerialAndWeb("Beacon Type: iBeacon");
              logToSerialAndWeb("   UUID:  " + String(beacon.uuid.c_str()));
              logToSerialAndWeb("   Major: " + String(beacon.major));
              logToSerialAndWeb("   Minor: " + String(beacon.minor));
              float beaconDistance = estimateDistance(beacon.txPower, rssi);
              logToSerialAndWeb("   Beacon Distance: ~" + String(beaconDistance, 2) + " m");

              sdLogger.writeIBeaconInfo(
                  String(beacon.uuid.c_str()),
                  String(beacon.major),
                  String(beacon.minor),
                  String(beaconDistance, 2),
                  manufacturerName,
                  manufacturerId,
                  rssi
              );
            }

            delay(100);

            // Analyze exposure and log results
            std::string mac = device->getAddress().toString().c_str();

            MACType macType = getMACType(mac);

            dev.mac = mac;
            dev.name = bestDeviceName.c_str();
            dev.manufacturer = manufacturerName.c_str();

            dev.isConnectable = is_connectable;

            dev.isPublicMac = isStaticPublicMAC(mac);
            dev.hasStaticMac = (macType == MACType::StaticRandom);
            dev.hasRotatingMac = isRotatingMAC(macType);

            dev.hasName = !dev.name.empty();
            dev.hasManufacturerData = !dev.manufacturer.empty();
            dev.hasCleartextData = containsCleartext(payloadVec);

            ExposureResult exposure = analyzeExposure(dev);

            logToSerialAndWeb("Uncovering Summary");
            logToSerialAndWeb("   Device Type: " + String(exposure.deviceType.c_str()));
            logToSerialAndWeb("   Identity Uncovering: " + String(exposure.identityExposure.c_str()));
            logToSerialAndWeb("   Tracking Risk: " + String(exposure.trackingRisk.c_str()));
            logToSerialAndWeb("   Privacy Level: " + String(exposure.privacyLevel.c_str()));
            logToSerialAndWeb(String("   Uncovering Tier: ") + tierToString(exposure.exposureTier));

            logToSerialAndWeb("\n   Reason:");
            for (auto& r : exposure.reasons) {
                logToSerialAndWeb("    - " + String(r.c_str()));
            }

            logToSerialAndWeb("----------------------------------");

            // Move to isTargetDevice to log on SD card
            sdLogger.writeDeviceInfo(
                address, 
                localName, 
                nameList, 
                manufacturerName.c_str(), 
                deviceInfoService
            );
            
            //logToSerialAndWeb("Write Data to SD Logger");
            delay(100);

            sdLogger.writeUncovered(exposure);

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
    logToSerialAndWeb("Scan Summary:");
    delay(100);
    logToSerialAndWeb("Sniffed:    " + String(targetConnects));
    delay(100);
    logToSerialAndWeb("Spotted:    " + String(allSpottedDevice));
    delay(100);
    logToSerialAndWeb("Suspicious: " + String(susDevice));
    delay(100);
    logToSerialAndWeb("Beacons:    " + String(beaconsFound));
    delay(100);
    
    logToSerialAndWeb("##########################\n");
    delay(100);

    scanIsRunning = false;
  }
}
