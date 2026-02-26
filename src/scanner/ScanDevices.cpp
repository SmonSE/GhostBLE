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

// Subscribe to all notifiable characteristics on a connected device
template<typename Callback>
void subscribeToAllNotifications(NimBLEClient* client, Callback notifyCallback) {
    if (!client) return;
    auto services = client->getServices(); // returns std::vector<NimBLERemoteService*>
    for (auto* service : services) {
        if (!service) continue;
        auto characteristics = service->getCharacteristics(true); // returns std::vector<NimBLERemoteCharacteristic*>
        for (auto* characteristic : characteristics) {
            if (!characteristic) continue;
            if (characteristic->canNotify()) {
                characteristic->subscribe(true, notifyCallback);
                Serial.printf("   Subscribed to notifis for char %s in service %s\n",
                    characteristic->getUUID().toString().c_str(),
                    service->getUUID().toString().c_str());
            }
        }
    }
}

// Example generic notification callback
void genericNotifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* data, size_t length, bool isNotify) {
    std::string uuid = pChar->getUUID().toString();
    Serial.printf("   [Notifis] Char UUID: %s, Data: ", uuid.c_str());
    String hexData;
    for (size_t i = 0; i < length; ++i) {
        Serial.printf("%02X ", data[i]);
        hexData += String(data[i], HEX);
        if (i < length - 1) hexData += " ";
    }

    String logLine = "   BLE Notify: UUID=" + String(uuid.c_str()) + ", Data=" + hexData;

    // Human-readable decoding for known characteristics
    if (uuid == "2a37" || uuid == "0x2a37") { // Heart Rate Measurement
      if (length >= 2) {
        uint8_t flags = data[0];
        uint16_t hr = 0;
        if (flags & 0x01) {
          if (length >= 3)
            hr = data[1] | (data[2] << 8);
        } else {
          hr = data[1];
        }
        Serial.printf(" | Heart Rate: %u bpm", hr);
        logLine += " | Heart Rate: " + String(hr) + " bpm";
      }
    } else if (uuid == "2a53" || uuid == "0x2a53") { // Step Count or Glucose Measurement (example, may differ)
      if (length >= 4) {
        uint32_t steps = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        Serial.printf(" | Step Count: %u", steps);
        logLine += " | Step Count: " + String(steps);
      } else if (length >= 2) {
        uint16_t glucose = data[1];
        Serial.printf(" | Glucose (raw): %u", glucose);
        logLine += " | Glucose (raw): " + String(glucose);
      }
    } else if (uuid == "2a19" || uuid == "0x2a19") { // Battery Level
      if (length >= 1) {
        Serial.printf(" | Battery Level: %u%%", data[0]);
        logLine += " | Battery Level: " + String(data[0]) + "%";
      }
    } else if (uuid == "2a2b" || uuid == "0x2a2b") { // Current Time
      if (length >= 7) {
        uint16_t year = data[0] | (data[1] << 8);
        uint8_t month = data[2];
        uint8_t day = data[3];
        uint8_t hour = data[4];
        uint8_t minute = data[5];
        uint8_t second = data[6];
        Serial.printf(" | Current Time: %04u-%02u-%02u %02u:%02u:%02u",
          year, month, day, hour, minute, second);
        logLine += " | Current Time: " + String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(minute) + ":" + String(second);
      }
    } else if (uuid == "2a1c" || uuid == "0x2a1c") { // Temperature Measurement
      if (length >= 5) {
        int32_t mantissa = (int32_t)(data[1] | (data[2] << 8) | (data[3] << 16));
        if (mantissa & 0x800000) mantissa |= 0xFF000000; // sign extend
        int8_t exponent = (int8_t)data[4];
        float temperature = mantissa * pow(10.0f, exponent);
        String unit = (data[0] & 0x01) ? "F" : "C";
        Serial.printf(" | Temperature: %.2f °%s", temperature, unit.c_str());
        logLine += " | Temperature: " + String(temperature, 2) + " °" + unit;
      }
    } else if (uuid == "2a6e" || uuid == "0x2a6e") { // Environmental Sensing: Temperature
      if (length >= 2) {
        int16_t tempRaw = (int16_t)(data[0] | (data[1] << 8));
        float temperature = tempRaw / 100.0f;
        Serial.printf(" | Env Temperature: %.2f °C", temperature);
        logLine += " | Env Temperature: " + String(temperature, 2) + " °C";
      }
    } else if (uuid == "2a6f" || uuid == "0x2a6f") { // Environmental Sensing: Humidity
      if (length >= 2) {
        uint16_t humRaw = (uint16_t)(data[0] | (data[1] << 8));
        float humidity = humRaw / 100.0f;
        Serial.printf(" | Humidity: %.2f%%", humidity);
        logLine += " | Humidity: " + String(humidity, 2) + "%";
      }
    } else if (uuid == "2a6d" || uuid == "0x2a6d") { // Environmental Sensing: Pressure
      if (length >= 4) {
        uint32_t presRaw = (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
        float pressure = presRaw / 10.0f;
        Serial.printf(" | Pressure: %.1f hPa", pressure);
        logLine += " | Pressure: " + String(pressure, 1) + " hPa";
      }
    } else if (uuid == "2a77" || uuid == "0x2a77") { // Illuminance
      if (length >= 2) {
        uint16_t luxRaw = (uint16_t)(data[0] | (data[1] << 8));
        Serial.printf(" | Illuminance: %u lux", luxRaw);
        logLine += " | Illuminance: " + String(luxRaw) + " lux";
      }
    } else if (uuid == "2a5c" || uuid == "0x2a5c") { // CO2 Concentration
      if (length >= 2) {
        uint16_t co2Raw = (uint16_t)(data[0] | (data[1] << 8));
        Serial.printf(" | CO2: %u ppm", co2Raw);
        logLine += " | CO2: " + String(co2Raw) + " ppm";
      }
    } else if (uuid == "2a4d" || uuid == "0x2a4d") { // HID Report
      if (length >= 1) {
        uint8_t reportId = data[0];
        Serial.printf(" | HID Report ID: %u", reportId);
        logLine += " | HID Report ID: " + String(reportId);
        if (length > 1) {
          Serial.printf(" | HID Data: ");
          String hidData;
          for (size_t i = 1; i < length; ++i) {
            Serial.printf("%02X ", data[i]);
            hidData += String(data[i], HEX);
            if (i < length - 1) hidData += " ";
          }
          logLine += " | HID Data: " + hidData;
        }
      }
    }

    Serial.println();
    sdLogger.writeCategory(logLine);
}

bool isTarget = false;

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
          pClient->setConnectTimeout(4 * 1000); // Set 4s timeout
      } else {
          Serial.println("⚠️ pClient is null! Cannot set connect timeout.");
      }

      if (pClient != nullptr) {
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

            if (!isGlassesTaskRunning && !isAngryTaskRunning) {
              xTaskCreatePinnedToCore(showGlassesExpressionTask, "BLEGlases", 4096, NULL, 0, NULL, 1);
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
                  xTaskCreatePinnedToCore(showAngryExpressionTask, "AngryFace", 4096, NULL, 4, NULL, 1);
                }
                isTarget = true;
                break;
              }
            }

            logToSerialAndWeb("Device Infos");
            logToSerialAndWeb(String("   Adress: " + address));
            logToSerialAndWeb(String("   Name:   " + localName));
            logToSerialAndWeb(String("   Manuf.: " + manufacturerName));
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
            dev.name = localName.c_str();
            dev.manufacturer = manufacturerName.c_str();

            dev.isConnectable = is_connectable;

            dev.isPublicMac = isStaticPublicMAC(mac);
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
                manufacturerName.c_str(), 
                deviceInfoService
            );

            // Subscribe to notifications for all characteristics that support it
            logToSerialAndWeb("Sub to Notifi all Chars");
            subscribeToAllNotifications(pClient, genericNotifyCallback);
            
            //logToSerialAndWeb("Write Data to SD Logger");
            delay(250);

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

            delay(250);
            logToSerialAndWeb("----------------------------------");

            sdLogger.writeUncovered(exposure);

            // Clear uuidList / nameList after Stored to SD Card
            uuidList.clear();
            nameList.clear();
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
            xTaskCreatePinnedToCore(showSadExpressionTask, "SadFace", 4096, NULL, 1, NULL, 1);
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
