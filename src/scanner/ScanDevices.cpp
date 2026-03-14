#include <NimBLEDevice.h>
#include <M5Cardputer.h>
#include <set>
#include <vector>
#include <algorithm>

#include "ScanDevices.h"
#include "../globals/globals.h"
#include "../helper/ManufacturerHelper.h"
#include "../helper/ServiceHelper.h"
#include "../helper/drawOverlay.h"
#include "../helper/showExpression.h"
#include "../logger/logger.h"
#include "../privacyCheck/devicePrivacy.h"
#include "../analyzer/ExposureAnalyzer.h"
#include "../models/DeviceInfo.h"
#include "../privacyCheck/ExposureClassifier.h"
#include "../helper/BLEDecoder.h"
#include "../GATTServices/genericAccessService.h"
#include "../GATTServices/batteryLevelService.h"
#include "../GATTServices/heartRateService.h"
#include "../GATTServices/temperatureService.h"
#include "../GATTServices/txPowerService.h"
#include "../GATTServices/currentTimeService.h"
#include "../GATTServices/immediateAlertService.h"
#include "../GATTServices/linkLossService.h"
#include "../GATTServices/pwnBeaconService.h"
#include "../analyzer/SecurityAnalyzer.h"
#include "../gps/GPSManager.h"
#include "../wardriving/WigleLogger.h"
#include "../helper/nibblesSpeech.h"

// Wardriving instances (defined in GhostBLE.ino)
extern GPSManager gpsManager;
extern WigleLogger wigleLogger;


NimBLEClient *pClient = nullptr;

// Subscribe to all notifiable characteristics on a connected device
template<typename Callback>
void subscribeToAllNotifications(NimBLEClient* client, Callback notifyCallback) {
    if (!client) return;
    auto services = client->getServices();
    for (auto* service : services) {
        if (!service) continue;
        auto characteristics = service->getCharacteristics(true);
        for (auto* characteristic : characteristics) {
          if (!characteristic) continue;
          if (characteristic->canNotify()) {
              characteristic->subscribe(true, notifyCallback);
              xpManager.awardXP(10);  // +10 XP: characteristic subscription
          } else if (characteristic->canIndicate()) {
              characteristic->subscribe(false, notifyCallback);
              xpManager.awardXP(10);  // +10 XP: characteristic subscription
          } else {
              continue;
          }
          if (characteristic->canRead()) {
              std::string val = characteristic->readValue();
              LOG(LOG_GATT, "   Read value length: " + String(val.length()));
          }
          LOG(LOG_GATT,
              String("   Subscribed to notifis for char ") +
              characteristic->getUUID().toString().c_str() +
              " in service " +
              service->getUUID().toString().c_str()
          );
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

// MAX_SEEN_DEVICES and MIN_FREE_HEAP_BYTES defined in config.h

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

void startBleScan() {
  LOG(LOG_SCAN, "▶️ Starting BLE scan...");
  scanIsRunning = false;   // allow scanForDevices() to trigger
}

void stopBleScan() {
  LOG(LOG_SCAN, "🛑 Stopping BLE scan...");
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
    out.reserve(data.length() * 3);

    for (uint8_t b : data)
    {
        if (b < 0x10) out += "0";
        out += String(b, HEX);
        out += " ";
    }

    out.toUpperCase();
    return out;
}

// ---------------------------------------------------------------------------
// Helper: parse advertised device info (address, name, manufacturer, iBeacon)
// Returns false if the device should be skipped (null or already seen).
// ---------------------------------------------------------------------------
static bool parseDeviceInfo(
    const NimBLEAdvertisedDevice* device,
    uint16_t& manufacturerId,
    String& manufacturerName,
    bool& isIBeacon,
    IBeaconInfo& beacon,
    bool& isPwnBeacon,
    PwnBeaconInfo& pwnBeacon,
    bool& hasCustomService,
    bool& hasWeakName,
    bool& isUnknownManufacturer,
    bool& isSecurityOrTrackingDevice,
    bool& proprietary)
{
  if (device == nullptr) {
    LOG(LOG_SYSTEM, "⚠️ device is null! Skipping.");
    return false;
  }

  addrStr = device->getAddress().toString();
  address = addrStr.c_str();
  localName = device->haveName() ? String(device->getName().c_str()) : "";
  rssi = device->getRSSI();
  is_connectable = device->isConnectable();

  // Dedupe: Insert into seenDevices immediately (before any connect attempt)
  if (seenDevices.find(addrStr) != seenDevices.end()) {
    LOG(LOG_SCAN, String("🛑 Already seen: ") + address.c_str() + "\n");
    return false;
  }
  seenDevices.insert(addrStr);

  // Risk factor: Weak/default device name
  if (localName == "< -- >" || localName == "BLE Device" || localName == "Random" ||
      localName.startsWith("ESP_") || localName.startsWith("ESP32") ||
      localName.startsWith("Arduino") || localName.startsWith("HM") ||
      localName.startsWith("HC-") || localName.startsWith("BLE")) {
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
      LOG(LOG_BEACON, "iBeacon detected!");
      LOG(LOG_BEACON, "   UUID:  " + String(beacon.uuid.c_str()));
      LOG(LOG_BEACON, "   Major: " + String(beacon.major));
      LOG(LOG_BEACON, "   Minor: " + String(beacon.minor));
      LOG(LOG_BEACON, "   TX:    " + String(beacon.txPower));
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

  // -------- Advertisement Service UUIDs --------
  int svcCount = device->getServiceUUIDCount();
  if (svcCount > 0) {
    LOG(LOG_SCAN, "   Advertised Services (" + String(svcCount) + "):");
    for (int s = 0; s < svcCount; s++) {
      NimBLEUUID svcUUID = device->getServiceUUID(s);
      std::string uuidStr = svcUUID.toString();
      // NimBLE returns "0x1800" for 16-bit UUIDs — strip the "0x" prefix
      String shortUUID = uuidStr.c_str();
      if (shortUUID.startsWith("0x")) {
        shortUUID = shortUUID.substring(2);
      }
      String svcName = getServiceName(shortUUID);
      LOG(LOG_SCAN, "     - " + shortUUID + " (" + svcName + ")");
    }
  }

  // -------- Advertisement TX Power --------
  if (device->haveTXPower()) {
    int8_t advTxPower = device->getTXPower();
    LOG(LOG_SCAN, "   Adv TX Power: " + String(advTxPower) + " dBm");
    float estDist = estimateDistance(advTxPower, device->getRSSI());
    if (estDist >= 0) {
      LOG(LOG_SCAN, "   Est. Distance (TX): ~" + String(estDist, 2) + " m");
    }
  }

  // -------- Advertisement Service Data (AD Type 0x16) --------
  int svcDataCount = device->getServiceDataCount();
  if (svcDataCount > 0) {
    LOG(LOG_SCAN, "   Service Data (" + String(svcDataCount) + "):");
    for (int sd = 0; sd < svcDataCount; sd++) {
      NimBLEUUID svcDataUUID = device->getServiceDataUUID(sd);
      std::string svcData = device->getServiceData(sd);

      String shortUUID = svcDataUUID.toString().c_str();
      if (shortUUID.startsWith("0x")) {
        shortUUID = shortUUID.substring(2);
      }
      String svcName = getServiceName(shortUUID);

      String hexData = bytesToHexString(svcData);
      LOG(LOG_SCAN, "     - UUID: " + shortUUID + " (" + svcName + ")");
      LOG(LOG_SCAN, "       Data: " + hexData);

      // Detect PwnBeacon service data
      if (svcDataUUID.equals(NimBLEUUID(PWNBEACON_SERVICE_UUID))) {
        pwnBeacon = PwnBeaconServiceHandler::parseAdvertisement((const uint8_t*)svcData.data(), svcData.length());
        if (pwnBeacon.valid) {
          isPwnBeacon = true;
          xpManager.awardXP(10);  // +10 XP: PwnBeacon detected
          LOG(LOG_BEACON, "👾 PwnBeacon detected!");
          LOG(LOG_BEACON, "   Name:     " + pwnBeacon.name);
          LOG(LOG_BEACON, "   Pwnd run: " + String(pwnBeacon.pwnd_run));
          LOG(LOG_BEACON, "   Pwnd tot: " + String(pwnBeacon.pwnd_tot));
          LOG(LOG_BEACON, "   FP:       " + PwnBeaconServiceHandler::fingerprintToString(pwnBeacon.fingerprint));
        }
      }
    }
  }

  return true;
}

// ---------------------------------------------------------------------------
// Helper: connect to device, discover GATT attributes, read characteristics.
// Returns true if a target device was detected (caller should break the loop).
// ---------------------------------------------------------------------------
static bool connectAndReadGATT(
    const NimBLEAdvertisedDevice* device,
    DeviceInfo& dev,
    bool& hasWritableChar)
{
  if (device->haveName()) {
      dev.advHasName = true;
  }

  deviceInfoService = DeviceInfoServiceHandler::readDeviceInfo(pClient);

  if (!deviceInfoService.isEmpty()) {
      dev.gattHasName = true;
  }

  // Read additional standard GATT services
  genericAccessService = GenericAccessServiceHandler::readGenericAccessInfo(pClient);
  batteryLevelService = BatteryServiceHandler::readBatteryLevel(pClient);
  heartRateService = HeartRateServiceHandler::readHeartRate(pClient);
  temperatureService = TemperatureServiceHandler::readTemperature(pClient);

  String txPowerInfo = TxPowerServiceHandler::readTxPowerLevel(pClient);
  if (!txPowerInfo.isEmpty()) {
      LOG(LOG_GATT, txPowerInfo);
  }

  // Read additional optional GATT services
  String timeInfo = CurrentTimeServiceHandler::readCurrentTime(pClient);
  if (!timeInfo.isEmpty()) {
      LOG(LOG_GATT, timeInfo);
  }

  String alertInfo = ImmediateAlertServiceHandler::readImmediateAlert(pClient);
  if (!alertInfo.isEmpty()) {
      LOG(LOG_GATT, alertInfo);
  }

  String linkLossInfo = LinkLossServiceHandler::readLinkLoss(pClient);
  if (!linkLossInfo.isEmpty()) {
      LOG(LOG_GATT, linkLossInfo);
  }

  LOG(LOG_GATT, "🔓 Connected and discovered attributes!");
  targetConnects++;
  xpManager.awardXP(5);  // +5 XP: GATT connection success

  if (!isGlassesTaskRunning && !isAngryTaskRunning) {
    if (xTaskCreatePinnedToCore(showGlassesExpressionTask, "BLEGlasses", 4096, NULL, 0, &glassesTaskHandle, 1) != pdPASS) {
      LOG(LOG_SYSTEM, "Failed to create BLEGlasses task");
      isGlassesTaskRunning = false;
    }
  }

  // Subscribe to notifications for all characteristics that support it
  LOG(LOG_GATT, "Sub to Notifi all Chars");
  subscribeToAllNotifications(pClient, genericNotifyCallback);

  for (auto it = pClient->getServices().begin(); it != pClient->getServices().end(); ++it) {
    NimBLERemoteService* service = *it;  // Dereference the iterator to get the element
    std::string serviceUuid = service->getUUID().toString();
    uuidList.push_back("Service UUID: " + std::string(serviceUuid.c_str()));

    for (auto cIt = service->getCharacteristics().begin(); cIt != service->getCharacteristics().end(); ++cIt) {
      NimBLERemoteCharacteristic* characteristic = *cIt;
      std::string charUuid = characteristic->getUUID().toString();
      uuidList.push_back("Characteristic UUID: " + std::string(charUuid.c_str()));

      if (charUuid == UUID_MODEL_NUMBER) {     // Model Number
          dev.gattHasModelInfo = true;
      }

      if (charUuid == UUID_MANUFACTURER_NAME || // Manufacturer Name
          charUuid == UUID_SERIAL_NUMBER) {    // Serial Number
          dev.gattHasIdentityInfo = true;
      }

      if (characteristic->canWrite() || characteristic->canWriteNoResponse()) {
          hasWritableChar = true;
      }

      // Read Characteristic User Description descriptor (0x2901)
      NimBLERemoteDescriptor* userDesc = characteristic->getDescriptor(NimBLEUUID("2901"));
      if (userDesc) {
          std::string descValue = userDesc->readValue();
          if (!descValue.empty() && isPrintableText(descValue)) {
              LOG(LOG_GATT, "     Descriptor [" + String(charUuid.c_str()) + "]: " + String(descValue.c_str()));
              nameList.push_back(descValue);
          }
      }

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

    if (device != nullptr && !device->getName().empty()) {
      localName = device->getName().c_str();
    }

    if (isTargetDevice(localName.c_str(), address.c_str(), serviceUuid.c_str(), deviceInfoService.c_str())) {
      targetFound = true;
      susDevice++;
      xpManager.awardXP(20);  // +20 XP: suspicious device found
      LOG(LOG_TARGET, "Target Message: !!! Target detected !!!");
      nibblesSpeechShow(SpeechContext::SUSPICIOUS);
      vTaskDelay(pdMS_TO_TICKS(2000));
      if (!isAngryTaskRunning) {
        //LOG(LOG_SYSTEM, "showAngryExpressionTask");
        if (xTaskCreatePinnedToCore(showAngryExpressionTask, "AngryFace", 4096, NULL, 4, &angryTaskHandle, 1) != pdPASS) {
          LOG(LOG_SYSTEM, "Failed to create AngryFace task");
          isAngryTaskRunning = false;
        }
      }
      isTarget = true;
      return true;  // target found — caller should break
    }
  }

  return false;  // no target — continue normally
}

// ---------------------------------------------------------------------------
// Helper: log exposure analysis results, then clear lists.
// ---------------------------------------------------------------------------
static void handleExposureResult(
    const ExposureResult& exposure,
    String& manufacturerName)
{
  LOG(LOG_PRIVACY, "Uncovering Summary");
  LOG(LOG_PRIVACY, "   Device Type: " + String(exposure.deviceType.c_str()));
  LOG(LOG_PRIVACY, "   Identity Uncovering: " + String(exposure.identityExposure.c_str()));
  LOG(LOG_PRIVACY, "   Tracking Risk: " + String(exposure.trackingRisk.c_str()));
  LOG(LOG_PRIVACY, "   Privacy Level: " + String(exposure.privacyLevel.c_str()));
  LOG(LOG_PRIVACY, String("   Uncovering Tier: ") + tierToString(exposure.exposureTier));

  LOG(LOG_PRIVACY, "\n   Reason:");
  for (auto& r : exposure.reasons) {
      LOG(LOG_PRIVACY, "    - " + String(r.c_str()));
  }

  LOG(LOG_PRIVACY, "----------------------------------");

  // Clear lists
  uuidList.clear();
  nameList.clear();
  localName.clear();
  manufacturerName.clear();
  deviceInfoService.clear();
}

void scanForDevices() {

  DeviceInfo dev;
  uint16_t manufacturerId = 0;
  String manufacturerName = "Unknown";

  NimBLEScan* pScan = NimBLEDevice::getScan();

  if (pScan == nullptr) {
    LOG(LOG_SYSTEM, "Scan instance creation failed.");
    return;
  }

  pScan->clearResults();        // 1. Clear previous results first
  pScan->setActiveScan(true);   // 2. Set active scan mode
  pScan->setInterval(BLE_SCAN_INTERVAL);
  pScan->setWindow(BLE_SCAN_WINDOW);
  delay(100);                   // Optional small delay for stability

  NimBLEScanResults results = pScan->getResults(3000);  // Scan 3 seconds to get scan results -> maybe check 3sec for smaller list and earlier new scan

  if (results.getCount() == 0) {
     LOG(LOG_SCAN, "NO DEVICES FOUND");
  } else {
    LOG(LOG_SCAN, "📲 DEVICES FOUND: " + String(results.getCount()));
    nibblesSpeechNotifyEvent();

    scanIsRunning = true;

    LOG(LOG_SCAN, "📡 Scan Is Running");

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
      bool isPwnBeaconDevice = false;
      PwnBeaconInfo pwnBeacon;

      if (!parseDeviceInfo(device, manufacturerId, manufacturerName,
                           isIBeacon, beacon, isPwnBeaconDevice, pwnBeacon,
                           hasCustomService, hasWeakName,
                           isUnknownManufacturer, isSecurityOrTrackingDevice,
                           proprietary)) {
        continue;
      }

      // --- Payload direkt holen ---
      std::vector<uint8_t> payloadVec = device->getPayload();
      std::string advData(payloadVec.begin(), payloadVec.end());

      // -------- Advertisement Flags Analysis (AD Type 0x01) --------
      std::vector<SecurityFinding> advFindings;
      analyzeAdvFlags(payloadVec, dev, advFindings);
      for (auto& f : advFindings) {
        LOG(LOG_SECURITY, "   [" + String(f.severity.c_str()) + "] " + String(f.description.c_str()));
      }

      handleDevicePrivacy(
          std::string(localName.c_str()),
          addrStr,
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
        LOG(LOG_SCAN, "   Device is not connectable");
      }

      pClient = NimBLEDevice::createClient();
      vTaskDelay(pdMS_TO_TICKS(200));

      if (!pClient) { // Make sure the client was created
        LOG(LOG_SYSTEM, "⚠️ Failed to create BLE client, skipping device.");
        continue;
      }

      if (seenDevices.size() >= MAX_SEEN_DEVICES || ESP.getFreeHeap() < MIN_FREE_HEAP_BYTES) {
        LOG(LOG_SYSTEM, "Clearing seenDevices (size: " + String(seenDevices.size()) +
                        ", free heap: " + String(ESP.getFreeHeap()) + ")");
        seenDevices.clear();
      }

      pClient->setConnectTimeout(4 * 1000); // Set 4s timeout

      {
        if (is_connectable && pClient->connect(*device)) {
          if (pClient->discoverAttributes()) {

            bool targetDetected = connectAndReadGATT(device, dev, hasWritableChar);

            if (!targetDetected) {
              LOG(LOG_SCAN, "Device Infos");
              LOG(LOG_SCAN, String("   Adress:  " + address));
              LOG(LOG_SCAN, String("   Name:    " + localName));
              LOG(LOG_SCAN, String("   Manuf.:  " + manufacturerName));
              LOG(LOG_SCAN, "   Device Name: ");
              String logLine;
              logLine.reserve(64);
              for (const auto& names : nameList) {
                if (!names.empty()) {
                  logLine = "     - ";
                  logLine += names.c_str();
                  LOG(LOG_SCAN, logLine);
                }
              }

              float distance = pow(10.0f, (float)(DISTANCE_CONSTANT - rssi) / RSSI_CONSTANT);
              LOG(LOG_SCAN, "Distance: " + String(distance, 2) + " m");
              LOG(LOG_SCAN, "     - RSSI: " + String(rssi));

              // iBeacon info
              if (isIBeacon) {
                beaconsFound++;
                LOG(LOG_BEACON, "Beacon Type: iBeacon");
                LOG(LOG_BEACON, "   UUID:  " + String(beacon.uuid.c_str()));
                LOG(LOG_BEACON, "   Major: " + String(beacon.major));
                LOG(LOG_BEACON, "   Minor: " + String(beacon.minor));
                float beaconDistance = estimateDistance(beacon.txPower, rssi);
                LOG(LOG_BEACON, "   Beacon Distance: ~" + String(beaconDistance, 2) + " m");
                LOG(LOG_BEACON, "   RSSI: " + String(rssi));
                LOG(LOG_BEACON, "   Manufacturer: " + manufacturerName);
              }

              // PwnBeacon info + GATT read
              if (isPwnBeaconDevice) {
                pwnbeaconsFound++;
                beaconsFound++;

                LOG(LOG_BEACON, "Beacon Type: PwnBeacon (PwnGrid/BLE)");
                LOG(LOG_BEACON, "   Name:     " + pwnBeacon.name);
                LOG(LOG_BEACON, "   Pwnd run: " + String(pwnBeacon.pwnd_run));
                LOG(LOG_BEACON, "   Pwnd tot: " + String(pwnBeacon.pwnd_tot));
                LOG(LOG_BEACON, "   FP:       " + PwnBeaconServiceHandler::fingerprintToString(pwnBeacon.fingerprint));

                // Read full identity, face, and name via GATT
                PwnBeaconServiceHandler::readGATT(pClient, pwnBeacon);

                LOG(LOG_BEACON, "   RSSI:     " + String(rssi));
              }

              // -------- Security Analysis --------
              SecurityResult secResult = analyzeDeviceSecurity(pClient, dev);

              dev.connectionEncrypted = secResult.connectionEncrypted;
              dev.hasWritableChars = (secResult.writableCharCount > 0);
              dev.writableCharCount = secResult.writableCharCount;
              dev.hasDFUService = secResult.hasDFUService;
              dev.hasUARTService = secResult.hasUARTService;
              dev.hasSensitiveUnencrypted = secResult.hasSensitiveServiceUnencrypted;
              dev.deviceFingerprint = secResult.deviceFingerprint;

              if (!secResult.findings.empty()) {
                LOG(LOG_SECURITY, "Security Findings:");
                for (auto& f : secResult.findings) {
                  LOG(LOG_SECURITY, "   [" + String(f.severity.c_str()) + "] " + String(f.description.c_str()));
                }
              }

              if (!secResult.deviceFingerprint.empty()) {
                LOG(LOG_SECURITY, "   Device Fingerprint: " + String(secResult.deviceFingerprint.c_str()));
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

              handleExposureResult(exposure, manufacturerName);
            }
          }
        } else {
            LOG(LOG_GATT, "🔒 Attribute discovery failed: " + address);

            dev.isConnectable = false;

            ExposureResult exposure = analyzeExposure(dev);

            if (!isGlassesTaskRunning && !isAngryTaskRunning && !isSadTaskRunning) {
              //LOG(LOG_SYSTEM, "showSadExpressionTask");
              if (xTaskCreatePinnedToCore(showSadExpressionTask, "SadFace", 4096, NULL, 1, &sadTaskHandle, 1) != pdPASS) {
                LOG(LOG_SYSTEM, "Failed to create SadFace task");
                isSadTaskRunning = false;
              }
            }

            handleExposureResult(exposure, manufacturerName);
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
    LOG(LOG_SCAN, "##########################");
    LOG(LOG_SCAN, "Scan Summary:");
    LOG(LOG_SCAN, "Sniffed:    " + String(targetConnects));
    LOG(LOG_SCAN, "Spotted:    " + String(allSpottedDevice));
    LOG(LOG_SCAN, "Suspicious: " + String(susDevice));
    LOG(LOG_SCAN, "Beacons:    " + String(beaconsFound));
    LOG(LOG_SCAN, "PwnBeacons: " + String(pwnbeaconsFound));
    if (wardrivingEnabled) {
      LOG(LOG_SCAN, "WiGLE log:  " + String(wigleLogger.getLoggedCount()));
      wigleLogger.flush();
    }
    LOG(LOG_SCAN, "##########################\n");

    xpManager.save();
    scanIsRunning = false;
  }
}
