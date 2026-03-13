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
#include "../helper/BLEDecoder.h"
#include "../gps/GPSManager.h"
#include "../wardriving/WigleLogger.h"

// Wardriving instances (defined in GhostBLE.ino)
extern GPSManager gpsManager;
extern WigleLogger wigleLogger;


NimBLEClient *pClient = nullptr;

// Forward declarations of required services/classes
class SDLogger;
SDLogger sdLogger;

// Subscribe to all notifiable characteristics on a connected device
template<typename Callback>
void subscribeToAllNotifications(NimBLEClient* client, Callback notifyCallback) {
  String logSubs = "Subscribing: " ;
    if (!client) return;
    auto services = client->getServices(); // returns std::vector<NimBLERemoteService*>
    for (auto* service : services) {
        if (!service) continue;
        auto characteristics = service->getCharacteristics(true); // returns std::vector<NimBLERemoteCharacteristic*>
        for (auto* characteristic : characteristics) {
          if (!characteristic) continue;
          if (characteristic->canNotify()) 
          {
            if (characteristic->canNotify()) {
                characteristic->subscribe(true, notifyCallback);
                xpManager.awardXP(10);  // +10 XP: characteristic subscription
            } else if (characteristic->canIndicate()) {
                characteristic->subscribe(false, notifyCallback);
                xpManager.awardXP(10);  // +10 XP: characteristic subscription
            }
            if (characteristic->canRead()) {
                std::string val = characteristic->readValue();
                logToSerialAndWeb(("   Read value length: " + String(val.length())).c_str());
                logSubs += "   Read value length: " + String(val.length());
            }
            logToSerialAndWeb(
                String("   Subscribed to notifis for char ") +
                characteristic->getUUID().toString().c_str() +
                " in service " +
                service->getUUID().toString().c_str()
            );
            logSubs += String("   Subscribed to notifis for char ") + 
                characteristic->getUUID().toString().c_str() +
                " in service " +
                service->getUUID().toString().c_str();
          }
          if (logSubs.length() > 0) {
            sdLogger.writeCategory(logSubs);
          }
          logSubs = "";
        }
    }
}

void genericNotifyCallback(NimBLERemoteCharacteristic* pChar,
                           uint8_t* data,
                           size_t length,
                           bool isNotify)
{
    if (!pChar || !data || length == 0) return;

    NimBLEUUID charUUID = pChar->getUUID();

    // Forward everything to the decoder
    decodeBLEData(charUUID.toString(), data, length);
}

bool isTarget = false;

#define MAX_SEEN_DEVICES 500
#define MIN_FREE_HEAP_BYTES 20000

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
        if (localName == "< -- >" || localName == "BLE Device" || localName == "Random") {
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
          xpManager.awardXP(2);  // +2 XP: manufacturer data decoded

          // Detect iBeacon
          beacon = parseIBeacon(mfg);
          if (beacon.valid) {
            isIBeacon = true;
            xpManager.awardXP(3);  // +3 XP: iBeacon parsed
            logToSerialAndWeb("iBeacon detected!");
            logToSerialAndWeb("   UUID:  " + String(beacon.uuid.c_str()));
            logToSerialAndWeb("   Major: " + String(beacon.major));
            logToSerialAndWeb("   Minor: " + String(beacon.minor));
            logToSerialAndWeb("   TX:    " + String(beacon.txPower));
          }

          if (manufacturerName.isEmpty()) {
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
      std::string advData(payloadVec.begin(), payloadVec.end());

      handleDevicePrivacy(
          std::string(localName.c_str()),
          std::string(address.c_str()),
          advData,
          payloadVec,
          is_connectable,
          dev
      );

      // for else ExposureAnalyzer
      dev.name = localName.c_str();
      dev.manufacturer = manufacturerName.c_str();

      isTarget = false;
      allSpottedDevice++;
      xpManager.awardXP(1);  // +1 XP: new device discovered

      // Print device connectability
      if (!is_connectable) {
        logToSerialAndWeb("   Device is not connectable");
      }

      pClient = NimBLEDevice::createClient();
      vTaskDelay(pdMS_TO_TICKS(200));

      if (!pClient) { // Make sure the client was created
        Serial.println("⚠️ Failed to create BLE client, skipping device.");
        continue;
      }

      if (seenDevices.size() >= MAX_SEEN_DEVICES || ESP.getFreeHeap() < MIN_FREE_HEAP_BYTES) {
        Serial.println("Clearing seenDevices (size: " + String(seenDevices.size()) +
                        ", free heap: " + String(ESP.getFreeHeap()) + ")");
        seenDevices.clear();
      }

      pClient->setConnectTimeout(4 * 1000); // Set 4s timeout

      {
        if (is_connectable && pClient->connect(*device)) {  
          if (pClient->discoverAttributes()) {

            if (device->haveName()) {
                dev.advHasName = true;
            }

            deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);

            if (!deviceInfoService.isEmpty()) {
                dev.gattHasName = true;
            }

            logToSerialAndWeb("🔓 Connected and discovered attributes!");
            targetConnects++;
            xpManager.awardXP(5);  // +5 XP: GATT connection success

            if (!isGlassesTaskRunning && !isAngryTaskRunning) {
              xTaskCreatePinnedToCore(showGlassesExpressionTask, "BLEGlasses", 4096, NULL, 0, &glassesTaskHandle, 1);
            }

            // Subscribe to notifications for all characteristics that support it
            logToSerialAndWeb("Sub to Notifi all Chars");
            subscribeToAllNotifications(pClient, genericNotifyCallback);
      
            for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
              NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
              std::string serviceUuid = service->getUUID().toString();
              uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

              for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
                NimBLERemoteCharacteristic* characteristic = *cIt;
                std::string charUuid = characteristic->getUUID().toString();
                uuidList.push_back("Characteristic UUID: " + std::string(charUuid.c_str()));

                if (charUuid == "2a24") {               // Model Number
                    dev.gattHasModelInfo = true;
                }

                if (charUuid == "2a29" ||               // Manufacturer Name
                    charUuid == "2a25") {               // Serial Number
                    dev.gattHasIdentityInfo = true;
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
              }

              if (isTargetDevice(localName.c_str(), address.c_str(), serviceUuid.c_str(), deviceInfoService.c_str())) {
                targetFound = true;
                susDevice++;
                xpManager.awardXP(20);  // +20 XP: suspicious device found
                logToSerialAndWeb("Target Message: !!! Target detected !!!");
                vTaskDelay(pdMS_TO_TICKS(2000));
                if (!isAngryTaskRunning) {
                  //logToSerialAndWeb("showAngryExpressionTask");
                  xTaskCreatePinnedToCore(showAngryExpressionTask, "AngryFace", 4096, NULL, 4, &angryTaskHandle, 1);
                }
                isTarget = true;
                break;
              }
            }

            logToSerialAndWeb("Device Infos");
            logToSerialAndWeb(String("   Adress:  " + address));
            logToSerialAndWeb(String("   Name:    " + localName));
            logToSerialAndWeb(String("   Manuf.:  " + manufacturerName));
            logToSerialAndWeb("   Device Name: ");
            for (const auto& names : nameList) {
              if (!names.empty()) {
                logToSerialAndWeb(String("     - ") + names.c_str());
              }
            }

            float distance = pow(10, (DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
            logToSerialAndWeb("Distance: " + String(distance, 2) + " m");
            logToSerialAndWeb("     - RSSI: " + String(rssi));

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
                  rssi
              );
            }

            // Analyze exposure and log results
            std::string mac = device->getAddress().toString().c_str();

            MACType macType = getMACType(mac);

            dev.mac = mac;
            dev.name = localName.c_str();
            dev.manufacturer = manufacturerName.c_str();

            dev.isConnectable = is_connectable;

            dev.isPublicMac = isUniversallyAdministeredMAC(mac);
            dev.hasStaticMac = (macType == MACType::StaticRandom);
            dev.hasRotatingMac = isRotatingMAC(macType);

            dev.hasName = !dev.name.empty();
            dev.hasManufacturerData = !dev.manufacturer.empty();
            dev.hasCleartextData = containsCleartext(payloadVec);

            ExposureResult exposure = analyzeExposure(dev);

            // Move to isTargetDevice to log on SD card
            sdLogger.writeDeviceInfo(
                address, 
                localName, 
                nameList, 
                deviceInfoService
            );
            
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

            sdLogger.writeUncovered(exposure);

            // Clear uuidList / nameList after Stored to SD Card
            uuidList.clear();
            nameList.clear();
            localName.clear();
            manufacturerName.clear();
            deviceInfoService.clear();
          }
        } else {
            logToSerialAndWeb("🔒 Attribute discovery failed: " + address);

            dev.isConnectable = false;

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
          if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
            //logToSerialAndWeb("showSadExpressionTask");
            xTaskCreatePinnedToCore(showSadExpressionTask, "SadFace", 4096, NULL, 1, &sadTaskHandle, 1);
          }

          // Move to isTargetDevice to log on SD card
          if (!localName.isEmpty())
          {
            sdLogger.writeDeviceInfo(
                  address, 
                  localName, 
                  nameList, 
                  deviceInfoService
            );
            sdLogger.writeUncovered(exposure);
          }

          // Clear uuidList / nameList after Stored to SD Card
          uuidList.clear();
          nameList.clear();
          localName.clear();
          manufacturerName.clear();
          deviceInfoService.clear();
        }
      }
      // Wardriving: log device with GPS coordinates
      if (wardrivingEnabled && gpsManager.isValid()) {
        wigleLogger.logDevice(
            address,
            localName.length() > 0 ? localName : String(dev.name.c_str()),
            rssi,
            gpsManager.getLatitude(),
            gpsManager.getLongitude(),
            gpsManager.getAltitude(),
            gpsManager.getHDOP(),
            gpsManager.getTimestamp()
        );
      }

      if (pClient != nullptr && pClient->isConnected()) {
        pClient->disconnect();
      }
      NimBLEDevice::deleteClient(pClient);
      pClient = nullptr;
    }
    logToSerialAndWeb("##########################");
    logToSerialAndWeb("Scan Summary:");
    logToSerialAndWeb("Sniffed:    " + String(targetConnects));
    logToSerialAndWeb("Spotted:    " + String(allSpottedDevice));
    logToSerialAndWeb("Suspicious: " + String(susDevice));
    logToSerialAndWeb("Beacons:    " + String(beaconsFound));
    if (wardrivingEnabled) {
      logToSerialAndWeb("WiGLE log:  " + String(wigleLogger.getLoggedCount()));
      wigleLogger.flush();
    }
    logToSerialAndWeb("##########################\n");

    xpManager.save();
    scanIsRunning = false;
  }
}
