#include "ble_scanner.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include "app/context/device_context.h"
#include "app/context/network_context.h"
#include "app/context/scan_context.h"
#include "app/context/ui_context.h"

#include "gattServices/notify_handler.h"

#include "web/web_sender.h"


// ---------------------------------------------------------------------------
//  Device registry — tracks seen devices across scan cycles.
//  Cleared reactively when heap runs low or registry grows too large.
// ---------------------------------------------------------------------------
DeviceRegistry registry;

// ---------------------------------------------------------------------------
//  Active NimBLE client — single instance, created/deleted per device.
//  Always set to nullptr after deleteClient() to avoid dangling pointer.
// ---------------------------------------------------------------------------
NimBLEClient* pClient = nullptr;

// ---------------------------------------------------------------------------
//  Tesla speech bubble messages — chosen at random on detection.
// ---------------------------------------------------------------------------
static const char* teslaMsgs[] = {
    "Oh! Tesla!",
    "Ooo Tesla!",
    "Hey Tesla!",
    "Sniff Tesla!",
    "Tesla ping!"
};
static constexpr int TESLA_MSG_COUNT = sizeof(teslaMsgs) / sizeof(teslaMsgs[0]);

// ---------------------------------------------------------------------------
//  Heart animation task flag.
//  atomic: heartTask runs on Core 1, read from Core 0.
// ---------------------------------------------------------------------------
static std::atomic<bool> heartTaskRunning{false};

// ===========================================================================
//  Structs
// ===========================================================================

// Parsed iBeacon advertisement payload.
struct IBeaconInfo {
    bool        valid    = false;
    std::string uuid;
    uint16_t    major    = 0;
    uint16_t    minor    = 0;
    int8_t      txPower  = 0;
};

// Extra Payload
void extractUUIDs(const std::vector<uint8_t>& payload) {
    int i = 0;

    while (i < payload.size()) {
        uint8_t len = payload[i++];
        if (len == 0 || i >= payload.size()) break;

        uint8_t type = payload[i++];

        // 🔹 16-bit UUIDs
        if (type == 0x02 || type == 0x03) {
            for (int j = 0; j < len - 1; j += 2) {
                if (i + j + 1 >= payload.size()) break;

                uint16_t uuid = payload[i + j] | (payload[i + j + 1] << 8);
                LOG(LOG_GATT, "16-bit UUID: " + String(uuid, HEX));
            }
        }

        // 🔹 128-bit UUIDs
        if (type == 0x06 || type == 0x07) {
            for (int j = 0; j < len - 1; j += 16) {
                if (i + j + 15 >= payload.size()) break;

                char buf[37];
                sprintf(buf,
                    "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                    payload[i+j+15], payload[i+j+14], payload[i+j+13], payload[i+j+12],
                    payload[i+j+11], payload[i+j+10],
                    payload[i+j+9],  payload[i+j+8],
                    payload[i+j+7],  payload[i+j+6],
                    payload[i+j+5],  payload[i+j+4],
                    payload[i+j+3],  payload[i+j+2],
                    payload[i+j+1],  payload[i+j]
                );

                LOG(LOG_GATT, "128-bit UUID: " + String(buf));
            }
        }

        i += (len - 1);
    }
}

// ===========================================================================
//  Helper: heart animation FreeRTOS task
// ===========================================================================
static void heartTask(void* param) {
    heartTaskRunning.store(true);

    drawHeart(30, 30, TFT_RED);
    drawHeart(45, 40, TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(3000));
    clearHearts();

    heartTaskRunning.store(false);
    vTaskDelete(NULL);
}

// ===========================================================================
//  Helper: parse iBeacon from raw manufacturer data.
//  Returns IBeaconInfo with valid=false if the data does not match iBeacon.
// ===========================================================================
static IBeaconInfo parseIBeacon(const std::string& mfg) {
    IBeaconInfo info;

    // Minimum iBeacon payload: 25 bytes
    if (mfg.size() < 25) return info;

    const uint8_t* d = (const uint8_t*)mfg.data();

    // Apple Company ID (0x004C) + iBeacon type/length (0x02 0x15)
    if (d[0] != 0x4C || d[1] != 0x00) return info;
    if (d[2] != 0x02 || d[3] != 0x15) return info;

    char uuidStr[37];
    snprintf(uuidStr, sizeof(uuidStr),
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        d[4],  d[5],  d[6],  d[7],
        d[8],  d[9],
        d[10], d[11],
        d[12], d[13],
        d[14], d[15], d[16], d[17], d[18], d[19]
    );

    info.uuid    = uuidStr;
    info.major   = (d[20] << 8) | d[21];
    info.minor   = (d[22] << 8) | d[23];
    info.txPower = (int8_t)d[24];
    info.valid   = true;

    return info;
}

// ===========================================================================
//  Helper: estimate physical distance from TX power and RSSI.
//  Returns -1.0 for invalid/unrealistic values.
// ===========================================================================
static float estimateDistance(int txPower, int rssi) {
    if (rssi == 0 || txPower == 0) return -1.0f;

    float ratio    = (float)rssi / (float)txPower;
    float distance = (ratio < 1.0f)
                   ? powf(ratio, 10.0f)
                   : 0.89976f * powf(ratio, 7.7095f) + 0.111f;

    // Discard unrealistic values (negative, NaN, infinity, >1000m)
    if (distance < 0.0f || distance > 1000.0f || isnan(distance) || isinf(distance))
        return -1.0f;

    return distance;
}

// ===========================================================================
//  Helper: check if a raw string is printable ASCII (32–126).
//  Used to filter binary garbage from GATT characteristic values.
// ===========================================================================
static bool isPrintableText(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (c < 32 || c > 126) return false;
    }
    return true;
}

// ===========================================================================
//  Helper: convert raw bytes to an uppercase hex string ("AA BB CC ...").
// ===========================================================================
static String bytesToHexString(const std::string& data) {
    String out;
    out.reserve(data.length() * 3);
    for (uint8_t b : data) {
        if (b < 0x10) out += "0";
        out += String(b, HEX);
        out += " ";
    }
    out.toUpperCase();
    return out;
}

// ===========================================================================
//  Scan control
// ===========================================================================

void startBleScan() {
    LOG(LOG_SCAN, "Starting BLE scan...");
    ScanContext::scanIsRunning.store(false);  // allow scanForDevices() to trigger
}

void stopBleScan() {
    LOG(LOG_SCAN, "Stopping BLE scan...");
    NimBLEDevice::getScan()->stop();
    ScanContext::bleScanEnabled.store(false);
}

// ===========================================================================
//  parseDeviceInfo
//
//  Extracts address, name, RSSI, manufacturer data, service UUIDs, iBeacon,
//  PwnBeacon, and advertisement service data from the advertised device.
//
//  Applies early-exit filters:
//    - null device
//    - own address (self-advertisement)
//    - RSSI below ignore threshold
//    - already seen MAC (dedup via DeviceRegistry)
//    - already seen fingerprint (dedup for rotating MACs)
//
//  Populates ScanContext strings and outDeviceSessionId.
//  Returns false if the device should be skipped entirely.
// ===========================================================================
static bool parseDeviceInfo(
    const NimBLEAdvertisedDevice* device,
    uint16_t&     manufacturerId,
    String&       manufacturerName,
    bool&         isIBeacon,
    IBeaconInfo&  beacon,
    bool&         isPwnBeacon,
    PwnBeaconInfo& pwnBeacon,
    bool&         hasCustomService,
    bool&         hasWeakName,
    bool&         isUnknownManufacturer,
    bool&         isSecurityOrTrackingDevice,
    bool&         proprietary,
    int&          outDeviceSessionId)
{
    // --- Guard: null device ---
    if (device == nullptr) {
        LOG(LOG_SYSTEM, "device is null — skipping.");
        return false;
    }

    // --- Guard: skip our own advertisement ---
    if (device->getAddress().equals(NimBLEDevice::getAddress())) {
        return false;
    }

    // --- Populate shared scan strings ---
    ScanContext::addrStr       = device->getAddress().toString();
    address                    = ScanContext::addrStr.c_str();
    localName                  = device->haveName() ? String(device->getName().c_str()) : "";
    ScanContext::rssi.store(device->getRSSI());
    ScanContext::is_connectable = device->isConnectable();

    // --- Guard: ignore devices below RSSI threshold ---
    if (ScanContext::rssi.load() <= RSSI_IGNORE_THRESHOLD) {
        LOG(LOG_SCAN, String("Ignoring far device: ")
            + String(ScanContext::addrStr.c_str())
            + " (" + String(ScanContext::rssi.load()) + " dBm)");
        return false;
    }

    // --- Dedup: skip already-seen MAC addresses ---
    if (!registry.isNewDevice(ScanContext::addrStr)) {
        LOG(LOG_SCAN, String("Already seen: ") + address.c_str());
        return false;
    }

    // --- Dedup: skip already-seen fingerprints (handles rotating MACs) ---
    bool isApple = (manufacturerId == 0x004C) || (manufacturerName == "Apple Inc.");
    SoftFingerprint fp = isApple
                       ? createAppleFingerprint(device, localName, ScanContext::rssi.load())
                       : createFingerprint(device);

    if (!registry.isNewFingerprint(fp)) {
        return false;
    }

    // --- Assign incremental session ID for cross-log correlation ---
    outDeviceSessionId = ScanContext::getOrAssignDeviceId(ScanContext::addrStr);
    devTag = "[#" + String(outDeviceSessionId) + "] ";

    // --- Risk factor: weak / default device name ---
    if (localName == "< -- >"       || localName == "BLE Device" ||
        localName == "Random"       || localName.startsWith("ESP_")  ||
        localName.startsWith("ESP32") || localName.startsWith("Arduino") ||
        localName.startsWith("HM")  || localName.startsWith("HC-") ||
        localName.startsWith("BLE")) {
        hasWeakName = true;
    }

    // --- Risk factor: sensitive device type inferred from name ---
    if (localName.indexOf("Tracker")  != -1 ||
        localName.indexOf("Tag")      != -1 ||
        localName.indexOf("Medical")  != -1 ||
        localName.indexOf("Security") != -1) {
        isSecurityOrTrackingDevice = true;
    }

    // --- Manufacturer data: decode ID, name, iBeacon ---
    if (device->haveManufacturerData()) {
        std::string mfg = device->getManufacturerData();

        if (mfg.size() >= 2) {
            manufacturerId   = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
        }
        manufacturerName = getManufacturerName(manufacturerId);
        DeviceContext::xpManager.awardXP(2.0f);  // +2.0 XP: manufacturer data decoded

        // iBeacon detection
        beacon = parseIBeacon(mfg);
        if (beacon.valid) {
            isIBeacon = true;
            DeviceContext::xpManager.awardXP(3.0f);  // +3.0 XP: iBeacon parsed

            LOG(LOG_BEACON, devTag + "iBeacon detected!\n"
                "   UUID:  " + String(beacon.uuid.c_str()) + "\n"
                "   Major: " + String(beacon.major) + "\n"
                "   Minor: " + String(beacon.minor) + "\n"
                "   TX:    " + String(beacon.txPower));

            // Tesla vehicles use a known iBeacon UUID
            if (String(beacon.uuid.c_str()).equalsIgnoreCase(TESLA_IBEACON_UUID)) {
                LOG(LOG_TARGET, devTag + "Tesla iBeacon detected");
            }
        }

        if (manufacturerName.isEmpty()) {
            isUnknownManufacturer = true;
        }
    }

    // --- Custom service UUID → likely proprietary protocol ---
    std::string serviceUuid = device->getServiceUUID().toString();
    if (!serviceUuid.empty()) {
        if (serviceUuid.length() > 8 && serviceUuid.find("0000") != 0) {
            hasCustomService = true;
            proprietary      = true;
        }
    }

    // --- Advertised service UUIDs ---
    int svcCount = device->getServiceUUIDCount();
    if (svcCount > 0) {
        String svcLog = devTag + "Advertised services (" + String(svcCount) + "):";

        for (int s = 0; s < svcCount; s++) {
            NimBLEUUID svcUUID  = device->getServiceUUID(s);
            String     shortUUID = svcUUID.toString().c_str();

            // NimBLE prefixes 16-bit UUIDs with "0x" — strip it
            if (shortUUID.startsWith("0x")) shortUUID = shortUUID.substring(2);

            svcLog += "\n     - " + shortUUID + " (" + getServiceName(shortUUID) + ")";

            // PwnBeacon detection via service UUID
            if (svcUUID.equals(NimBLEUUID(PWNBEACON_SERVICE_UUID))) {
                isPwnBeacon = true;
                DeviceContext::beaconsFound++;
                DeviceContext::pwnbeaconsFound++;
                if (!heartTaskRunning.load()) {
                    xTaskCreatePinnedToCore(heartTask, "Heart", 2048, NULL, 1, NULL, 1);
                }
                LOG(LOG_BEACON, devTag + "PwnBeacon detected (service UUID)!");
            }
        }
        LOG(LOG_GATT, svcLog);
    }

    // --- TX Power + distance estimate ---
    if (device->haveTXPower()) {
        int8_t advTxPower = device->getTXPower();
        String txLog      = devTag + "Adv TX power: " + String(advTxPower) + " dBm";

        float estDist = estimateDistance(advTxPower, ScanContext::rssi.load());
        if (estDist >= 0.0f) {
            txLog += "\n   Est. distance (TX): ~" + String(estDist, 2) + " m";
        }
        LOG(LOG_SCAN, txLog);
    }

    // --- Advertisement service data (AD type 0x16) ---
    int svcDataCount = device->getServiceDataCount();
    if (svcDataCount > 0) {
        String sdLog = devTag + "Service data (" + String(svcDataCount) + "):";

        for (int sd = 0; sd < svcDataCount; sd++) {
            NimBLEUUID  svcDataUUID = device->getServiceDataUUID(sd);
            std::string svcData     = device->getServiceData(sd);
            String      shortUUID   = svcDataUUID.toString().c_str();

            if (shortUUID.startsWith("0x")) shortUUID = shortUUID.substring(2);

            sdLog += "\n     - UUID: " + shortUUID
                   + " (" + getServiceName(shortUUID) + ")"
                   + "\n       Data: " + bytesToHexString(svcData);

            // PwnBeacon service data payload
            if (svcDataUUID.equals(NimBLEUUID(PWNBEACON_SERVICE_UUID))) {
                pwnBeacon = PwnBeaconServiceHandler::parseAdvertisement(
                    (const uint8_t*)svcData.data(), svcData.length());

                if (pwnBeacon.valid) {
                    isPwnBeacon = true;
                    DeviceContext::xpManager.awardXP(1.0f);  // +1.0 XP: PwnBeacon detected

                    LOG(LOG_BEACON, devTag + "PwnBeacon detected!\n"
                        "   Name:     " + pwnBeacon.name + "\n"
                        "   Pwnd run: " + String(pwnBeacon.pwnd_run) + "\n"
                        "   Pwnd tot: " + String(pwnBeacon.pwnd_tot) + "\n"
                        "   FP:       " + PwnBeaconServiceHandler::fingerprintToString(pwnBeacon.fingerprint));
                }
            }
        }
        LOG(LOG_GATT, sdLog);
    }

    return true;
}

// ===========================================================================
//  connectAndReadGATT
//
//  Connects to the device, discovers all GATT attributes, reads
//  characteristics and descriptors, runs registered service handlers,
//  and performs target detection.
//
//  Populates dev with connection-derived metadata.
//  Returns true if a target device was detected (caller should break).
// ===========================================================================
static bool connectAndReadGATT(
    const NimBLEAdvertisedDevice* device,
    DeviceInfo&   dev,
    bool&         hasWritableChar,
    const String& devTag,
    int           remaining)
{
    if (device->haveName()) dev.advHasName = true;

    LOG(LOG_GATT, devTag + "Connected and discovered attributes: " + address);

    // Run all registered GATT service handlers (DeviceInfo, Battery, etc.)
    String serviceOutput = GATTServiceRegistry::runDiscoveredHandlers(pClient);

    // After Registry-Run, before Security-Analyse:
    // Short window if many devices in queue:
    uint32_t captureMs = (remaining > 5) ? 1500 : 2500;
    String notifySummary = NotifyHandler::subscribeAndCapture(pClient, captureMs);
    if (!notifySummary.isEmpty()) {
        LOG(LOG_NOTIFY, devTag + notifySummary);
        dev.hasNotifyData = true;
        dev.notifyCharCount  = NotifyHandler::lastNotifyCount();
    }

    // Cache Device Information Service result for downstream use
    deviceInfoService = GATTServiceRegistry::getLastResult("180a");
    if (!deviceInfoService.isEmpty()) dev.gattHasName = true;

    ScanContext::targetConnects++;
    DeviceContext::xpManager.awardXP(0.5f);  // +0.5 XP: GATT connection success

    // --- Iterate services and characteristics ---
    for (auto svcIt = pClient->getServices().begin();
         svcIt != pClient->getServices().end(); ++svcIt)
    {
        NimBLERemoteService* service     = *svcIt;
        std::string          serviceUuid = service->getUUID().toString();

        ScanContext::uuidList.push_back("Service UUID: " + serviceUuid);

        for (auto cIt = service->getCharacteristics().begin();
             cIt != service->getCharacteristics().end(); ++cIt)
        {
            NimBLERemoteCharacteristic* characteristic = *cIt;
            std::string charUuid = characteristic->getUUID().toString();

            ScanContext::uuidList.push_back("Characteristic UUID: " + charUuid);

            // Flag device info fields found via GATT
            if (charUuid == UUID_MODEL_NUMBER)                            dev.gattHasModelInfo    = true;
            if (charUuid == UUID_MANUFACTURER_NAME || charUuid == UUID_SERIAL_NUMBER) dev.gattHasIdentityInfo = true;

            // Track writable characteristics (potential attack surface)
            if (characteristic->canWrite() || characteristic->canWriteNoResponse()) {
                hasWritableChar = true;
            }

            // Read User Description descriptor (0x2901) for human-readable label
            NimBLERemoteDescriptor* userDesc = characteristic->getDescriptor(NimBLEUUID("2901"));
            if (userDesc) {
                std::string descValue = userDesc->readValue();
                if (!descValue.empty() && isPrintableText(descValue)) {
                    LOG(LOG_GATT, devTag + "Descriptor [" + String(charUuid.c_str()) + "]: "
                        + String(descValue.c_str()));
                    ScanContext::nameList.push_back(descValue);
                    DeviceContext::xpManager.awardXP(1.0f);  // +1.0 XP: known characteristic decoded
                }
            }

            // Read characteristic value — only process printable ASCII
            std::string rawValue = characteristic->readValue();
            if (!rawValue.empty() && isPrintableText(rawValue)) {
                dev.gattHasName = true;
                ScanContext::nameList.push_back(rawValue);

                if (looksLikePersonalName(rawValue))    dev.gattHasPersonalName  = true;
                if (looksLikeIdentityData(rawValue))    dev.gattHasIdentityInfo  = true;
                if (looksLikeEnvironmentName(rawValue)) dev.gattHasEnvironmentName = true;
            }
        }

        // Refresh local name from device if available
        if (device != nullptr && !device->getName().empty()) {
            localName = device->getName().c_str();
        }

        // --- Tesla detection via GATT service UUID ---
        if (isTeslaDevice("", serviceUuid.c_str())) {
            LOG(LOG_TARGET, devTag + "Tesla vehicle detected via GATT service");
            nibblesSpeechShowCustom(teslaMsgs[random(TESLA_MSG_COUNT)]);
        }

        // --- Known / suspicious target detection ---
        if (isTargetDevice(localName.c_str(), address.c_str(),
                           serviceUuid.c_str(), deviceInfoService.c_str())) {
            ScanContext::targetFound = true;
            ScanContext::susDevice++;
            DeviceContext::xpManager.awardXP(2.0f);  // +2.0 XP: suspicious device found

            LOG(LOG_TARGET, devTag + "!!! Target detected !!!");
            nibblesSpeechShow(SpeechContext::SUSPICIOUS);
            vTaskDelay(pdMS_TO_TICKS(2000));

            if (!UIContext::isAngryTaskRunning.load()) {
                if (xTaskCreatePinnedToCore(showAngryExpressionTask, "AngryFace",
                    4096, NULL, 5, &UIContext::angryTaskHandle, 1) != pdPASS) {
                    LOG(LOG_SYSTEM, "Failed to create AngryFace task");
                    UIContext::isAngryTaskRunning.store(false);
                }
            }
            return true;  // target found — caller breaks loop
        }else {
          ScanContext::targetFound = false;  // ← Only reset if NOT a target
        }
    }

    return false;  // no target found — continue
}

// ===========================================================================
//  handleExposureResult
//
//  Logs the full exposure / privacy analysis result, then clears all
//  temporary per-device string lists to free RAM for the next device.
// ===========================================================================
static void handleExposureResult(
    const ExposureResult& exposure,
    String&       manufacturerName,
    const String& devTag)
{
    String privLog = devTag + "Exposure summary\n"
        "   Device type:        " + String(exposure.deviceType.c_str()) + "\n"
        "   Identity exposure:  " + String(exposure.identityExposure.c_str()) + "\n"
        "   Tracking risk:      " + String(exposure.trackingRisk.c_str()) + "\n"
        "   Privacy level:      " + String(exposure.privacyLevel.c_str()) + "\n"
        "   Exposure tier:      " + tierToString(exposure.exposureTier) + "\n"
        "   Reasons:";

    for (auto& r : exposure.reasons) {
        privLog += "\n    - " + String(r.c_str());
    }
    privLog += "\n----------------------------------";
    LOG(LOG_PRIVACY, privLog);

    // Clear per-device temporary data to prevent leaking into next scan entry
    ScanContext::uuidList.clear();
    ScanContext::nameList.clear();
    localName.clear();
    manufacturerName.clear();
    deviceInfoService.clear();
}

// ===========================================================================
//  scanForDevices
//
//  Main scan loop — called from scanTask() on Core 1.
//
//  Flow:
//    1. Start passive NimBLE scan (4 seconds)
//    2. Restart PwnBeacon advertising (scanning pauses it)
//    3. For each discovered device:
//       a. parseDeviceInfo()   — advertisement layer
//       b. Privacy / exposure analysis
//       c. Connect + readGATT  — if signal strong enough
//       d. Security analysis
//       e. Full exposure analysis + logging
//       f. Wardriving GPS log  — if enabled and GPS fix available
//    4. Print scan summary
//    5. Save XP to SD card
// ===========================================================================
void scanForDevices() {
    DeviceInfo dev;
    uint16_t   manufacturerId   = 0;
    String     manufacturerName = "Unknown";

    ScanContext::scanIsRunning.store(true);

    // --- Configure and run NimBLE scan ---
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan == nullptr) {
        LOG(LOG_SYSTEM, "Scan instance creation failed.");
        ScanContext::scanIsRunning.store(false);
        return;
    }

    pScan->clearResults();
    pScan->setActiveScan(true);
    pScan->setInterval(BLE_SCAN_INTERVAL);
    pScan->setWindow(BLE_SCAN_WINDOW);
    delay(100);  // brief stability delay before scan

    NimBLEScanResults results = pScan->getResults(4000);  // 4-second scan window

    // Restart PwnBeacon advertising (NimBLE stops advertising during scan)
    PwnBeaconServiceHandler::updateCounters(
        ScanContext::targetConnects.load(),
        ScanContext::allSpottedDevice.load());

    // Brief advertising window before processing — lets peers discover us
    vTaskDelay(pdMS_TO_TICKS(ADV_WINDOW_MS));

    if (results.getCount() == 0) {
        LOG(LOG_SCAN, "No devices found.");
        ScanContext::scanIsRunning.store(false);
        return;
    }

    LOG(LOG_SCAN, "Devices found: " + String(results.getCount()));
    nibblesSpeechNotifyEvent();

    // Trigger happy expression at start of fruitful scan
    if (!UIContext::isHappyTaskRunning.load()) {
        if (xTaskCreatePinnedToCore(showHappyExpressionTask, "HappyFace",
            4096, NULL, 2, &UIContext::happyTaskHandle, 1) != pdPASS) {
            LOG(LOG_SYSTEM, "Failed to create HappyFace task");
            UIContext::isHappyTaskRunning.store(false);
        }
    }

    // ===================================================================
    //  Per-device processing loop
    // ===================================================================
    for (int i = 0; i < results.getCount(); i++) {
        const NimBLEAdvertisedDevice* device = results.getDevice(i);

        showFindingCounter(
            ScanContext::targetConnects.load(),
            ScanContext::susDevice.load(),
            ScanContext::allSpottedDevice.load());

        // --- Per-device risk flags (reset each iteration) ---
        bool hasCustomService          = false;
        bool hasWeakName               = false;
        bool isUnknownManufacturer     = false;
        bool isSecurityOrTrackingDevice = false;
        bool hasWritableChar           = false;
        bool proprietary               = false;
        bool isIBeacon                 = false;
        bool isPwnBeaconDevice         = false;
        int  devSessionId              = 0;

        IBeaconInfo   beacon;
        PwnBeaconInfo pwnBeacon;

        // --- Advertisement layer parsing + early filters ---
        if (!parseDeviceInfo(device, manufacturerId, manufacturerName,
                             isIBeacon, beacon, isPwnBeaconDevice, pwnBeacon,
                             hasCustomService, hasWeakName,
                             isUnknownManufacturer, isSecurityOrTrackingDevice,
                             proprietary, devSessionId)) {
            continue;
        }

        String devTag = "[#" + String(devSessionId) + "] ";

        // Raw advertisement payload for privacy/flag analysis
        std::vector<uint8_t> payloadVec = device->getPayload();
        std::string          advData(payloadVec.begin(), payloadVec.end());

        // --- Advertisement flags security analysis (AD type 0x01) ---
        std::vector<SecurityFinding> advFindings;
        analyzeAdvFlags(payloadVec, dev, advFindings);
        if (!advFindings.empty()) {
            String advSecLog = devTag + "Adv security flags:";
            for (auto& f : advFindings) {
                advSecLog += "\n   [" + String(f.severity.c_str()) + "] "
                           + String(f.description.c_str());
            }
            LOG(LOG_SECURITY, advSecLog);
        }

        // --- Privacy analysis (MAC type, cleartext, rotating address) ---
        handleDevicePrivacy(
            std::string(localName.c_str()),
            ScanContext::addrStr,
            advData,
            payloadVec,
            ScanContext::is_connectable,
            dev,
            devTag);

        // Hier muss nach Apple Name device gecheckt werden
        // --- Apple model resolution ---
        // Scan nameList for Apple model identifiers (e.g. "iPhone17,3")
        String modelIdentifier;
        for (const auto& n : ScanContext::nameList) {
            String s = n.c_str();
            if ((s.startsWith("iPhone") || s.startsWith("iPad") || s.startsWith("Mac"))
                && s.indexOf(",") != -1) {
                modelIdentifier = s;
                break;
            }
        }

        String modelName;
        if (!modelIdentifier.isEmpty()) {
            modelName    = getAppleModelName(modelIdentifier);
            displayName  = modelName;
            dev.name = std::string(modelName.c_str()); 
            dev.displayName = std::string(modelName.c_str()); 
        } else if (manufacturerName == "Apple Inc.") {
            modelName         = "Apple Device";
            dev.displayName   = "Apple Device";  
        } else {
            dev.name          = localName.c_str();  
            dev.manufacturer  = manufacturerName.c_str();
        } 

        ScanContext::allSpottedDevice++;
        DeviceContext::xpManager.awardXP(0.1f);  // +0.1 XP: new device discovered

        // --- Tesla detection from advertisement name (no connection needed) ---
        if (isTeslaDevice(localName, "")) {
            LOG(LOG_TARGET, devTag + "Tesla vehicle detected: " + localName);
            String teslaMsg = localName.startsWith("Tesla ") ? localName : "Tesla found!";
            nibblesSpeechShowCustom(teslaMsg.c_str());
        }

        if (!ScanContext::is_connectable) {
            LOG(LOG_SCAN, devTag + "Device is not connectable.");
        }

        // --- RSSI threshold: skip GATT connection for weak signals ---
        int  currentRSSI   = ScanContext::rssi.load();
        bool shouldConnect = (currentRSSI >= RSSI_CONNECT_THRESHOLD);

        if (!shouldConnect) {
            LOG(LOG_SCAN, devTag + "Weak signal — scan only, no connect: "
                + String(currentRSSI) + " dBm");

            // Still run a bare-minimum exposure analysis with advertisement data
            dev.isConnectable = false;
            ExposureResult exposure = analyzeExposure(dev);
            handleExposureResult(exposure, manufacturerName, devTag);

            WebSender::sendDevice(dev, devSessionId, currentRSSI,
                      isIBeacon, isPwnBeaconDevice,
                      dev.hasNotifyData);

            // Sad expression: device visible but unreachable
            if (!UIContext::isAngryTaskRunning.load() && !UIContext::isSadTaskRunning.load()) {
                if (xTaskCreatePinnedToCore(showSadExpressionTask, "SadFace",
                    4096, NULL, 3, &UIContext::sadTaskHandle, 1) != pdPASS) {
                    LOG(LOG_SYSTEM, "Failed to create SadFace task");
                    UIContext::isSadTaskRunning.store(false);
                }
            }
            continue;
        }

        // --- Reactive registry cleanup before allocating a new client ---
        if (registry.size() >= MAX_SEEN_DEVICES || ESP.getFreeHeap() < MIN_FREE_HEAP_BYTES) {
            LOG(LOG_SYSTEM, "Clearing registry (size: " + String(registry.size())
                + ", free heap: " + String(ESP.getFreeHeap()) + ")");
            registry.clear();
            ScanContext::deviceSessionMap.clear();
        }

        // --- Create NimBLE client ---
        pClient = NimBLEDevice::createClient();
        if (!pClient) {
            LOG(LOG_SYSTEM, "Failed to create BLE client — skipping device.");
            continue;
        }
        pClient->setConnectTimeout(3 * 1000);  // 3-second connection timeout

        vTaskDelay(pdMS_TO_TICKS(200));  // brief delay for stable connection

        // ---------------------------------------------------------------
        //  GATT connection branch
        // ---------------------------------------------------------------
        if (ScanContext::is_connectable && pClient->connect(*device)) {

            // get Device scan count
            int remaining = results.getCount() - i;
          
            if (pClient->discoverAttributes()) {
                // --- Full GATT read + target detection ---
                bool targetDetected = connectAndReadGATT(device, dev, hasWritableChar, devTag, remaining);
                if (!targetDetected) {
                    LOG(LOG_GATT, devTag + "No target detected via GATT: " + address);
                }

                // --- Apple model resolution ---
                // Scan nameList for Apple model identifiers (e.g. "iPhone17,3")
                String modelIdentifier;
                for (const auto& n : ScanContext::nameList) {
                    String s = n.c_str();
                    if ((s.startsWith("iPhone") || s.startsWith("iPad") || s.startsWith("Mac"))
                        && s.indexOf(",") != -1) {
                        modelIdentifier = s;
                        break;
                    }
                }

                String modelName;
                if (!modelIdentifier.isEmpty()) {
                    modelName    = getAppleModelName(modelIdentifier);
                    displayName  = modelName;
                    dev.displayName = std::string(modelName.c_str()); 
                } else if (manufacturerName == "Apple Inc.") {
                    modelName         = "Apple Device";
                    dev.displayName   = "Apple Device";
                    dev.name          = localName.c_str();
                    dev.manufacturer  = manufacturerName.c_str();
                }

                // --- Build and log device info summary ---
                String infoLog = devTag + "Device info\n"
                    "   Address:  " + address + "\n"
                    "   Name:     " + localName + "\n"
                    "   Manuf.:   " + manufacturerName;

                if (!modelName.isEmpty()) infoLog += "\n   Model:    " + modelName;

                infoLog += "\n   Raw GATT:";
                for (const auto& n : ScanContext::nameList) {
                    if (!n.empty()) infoLog += "\n     - " + String(n.c_str());
                }

                float distance = powf(10.0f,
                    (float)(DISTANCE_CONSTANT - currentRSSI) / (float)RSSI_CONSTANT);
                infoLog += "\n   Distance: ~" + String(distance, 2) + " m"
                         + "\n   RSSI:     " + String(currentRSSI) + " dBm";
                LOG(LOG_GATT, infoLog);

                // --- iBeacon details ---
                if (isIBeacon) {
                    DeviceContext::beaconsFound++;
                    float beaconDist = estimateDistance(beacon.txPower, currentRSSI);
                    LOG(LOG_BEACON, devTag + "Beacon type: iBeacon\n"
                        "   UUID:     " + String(beacon.uuid.c_str()) + "\n"
                        "   Major:    " + String(beacon.major) + "\n"
                        "   Minor:    " + String(beacon.minor) + "\n"
                        "   Distance: ~" + String(beaconDist, 2) + " m\n"
                        "   RSSI:     " + String(currentRSSI) + " dBm\n"
                        "   Manuf.:   " + manufacturerName);
                }

                // --- PwnBeacon: full GATT read ---
                if (isPwnBeaconDevice) {
                    PwnBeaconServiceHandler::readGATT(pClient, pwnBeacon);
                    LOG(LOG_BEACON, devTag + "Beacon type: PwnBeacon\n"
                        "   Name:     " + pwnBeacon.name + "\n"
                        "   Pwnd run: " + String(pwnBeacon.pwnd_run) + "\n"
                        "   Pwnd tot: " + String(pwnBeacon.pwnd_tot) + "\n"
                        "   FP:       " + PwnBeaconServiceHandler::fingerprintToString(pwnBeacon.fingerprint) + "\n"
                        "   RSSI:     " + String(currentRSSI) + " dBm");
                }

                // --- Security analysis (writable chars, DFU, UART, encryption) ---
                SecurityResult secResult = analyzeDeviceSecurity(pClient, dev);

                dev.connectionEncrypted      = secResult.connectionEncrypted;
                dev.hasWritableChars         = (secResult.writableCharCount > 0);
                dev.writableCharCount        = secResult.writableCharCount;
                dev.hasDFUService            = secResult.hasDFUService;
                dev.hasUARTService           = secResult.hasUARTService;
                dev.hasSensitiveUnencrypted  = secResult.hasSensitiveServiceUnencrypted;
                dev.deviceFingerprint        = secResult.deviceFingerprint;

                if (!secResult.findings.empty() || !secResult.deviceFingerprint.empty()) {
                    String secLog = devTag + "Security findings:";
                    for (auto& f : secResult.findings) {
                        secLog += "\n   [" + String(f.severity.c_str()) + "] "
                                + String(f.description.c_str());

                        // Increment per-category security counters for the scan summary
                        if (f.severity  == "HIGH")                   ScanContext::highFindingsCount++;
                        if (f.category  == "SENSITIVE_UNENCRYPTED")  ScanContext::unencryptedSensitiveCount++;
                        if (f.category  == "WRITABLE_NO_AUTH")       ScanContext::writableNoAuthCount++;
                    }
                    if (!secResult.deviceFingerprint.empty()) {
                        secLog += "\n   Fingerprint: " + String(secResult.deviceFingerprint.c_str());
                    }
                    LOG(LOG_SECURITY, secLog);
                }

                // --- Full exposure analysis ---
                MACType macType = getMACType(device->getAddress().toString().c_str());

                dev.mac              = device->getAddress().toString().c_str();
                dev.isConnectable    = ScanContext::is_connectable;
                dev.isPublicMac      = (macType == MACType::Public);
                dev.hasStaticMac     = (macType == MACType::Public || macType == MACType::StaticRandom);
                dev.hasRotatingMac   = isRotatingMAC(macType);
                dev.hasName          = !dev.name.empty();
                dev.hasManufacturerData = !dev.manufacturer.empty();
                dev.hasCleartextData = containsCleartext(payloadVec);

                ExposureResult exposure = analyzeExposure(dev);
                handleExposureResult(exposure, manufacturerName, devTag);

                WebSender::sendDevice(dev, devSessionId, currentRSSI,
                      isIBeacon, isPwnBeaconDevice,
                      dev.hasNotifyData);

                // Glasses expression: detective mode after successful GATT read
                if (!UIContext::isGlassesTaskRunning.load() && !UIContext::isAngryTaskRunning.load()) {
                    if (xTaskCreatePinnedToCore(showGlassesExpressionTask, "BLEGlasses",
                        4096, NULL, 4, &UIContext::glassesTaskHandle, 1) != pdPASS) {
                        LOG(LOG_SYSTEM, "Failed to create BLEGlasses task");
                        UIContext::isGlassesTaskRunning.store(false);
                    }
                }

                delay(1000);  // brief pause for stable UI flow

            } else {
                // Connected but attribute discovery failed (device likely rejected)
                LOG(LOG_GATT, devTag + "Connected but attribute discovery failed: " + address);
            }

        } else {
          // ---------------------------------------------------------------
          //  Connection failed branch
          // ---------------------------------------------------------------
          if (device != nullptr) {
              if (device->haveServiceUUID()) {
                NimBLEUUID uuid = device->getServiceUUID();
                String uuidStr = String(uuid.toString().c_str());

                LOG(LOG_GATT, devTag + "Service-UUID: " + uuidStr);

                // --- Known / suspicious target detection (advertisement-based) ---
                // Note: deviceInfoService are empty here since we didn't connect
                if (isTargetDevice(localName.c_str(), address.c_str(), uuidStr.c_str(), "")) {
                    ScanContext::targetFound = true;
                    ScanContext::susDevice++;
                    DeviceContext::xpManager.awardXP(2.0f);  // +2.0 XP: suspicious device found

                    LOG(LOG_TARGET, devTag + "!!! Target detected (no GATT) !!!");
                    LOG(LOG_GATT, devTag + "!!! Target detected (no GATT) !!!");
                    nibblesSpeechShow(SpeechContext::SUSPICIOUS);
                    vTaskDelay(pdMS_TO_TICKS(2000));

                    if (!UIContext::isAngryTaskRunning.load()) {
                        if (xTaskCreatePinnedToCore(showAngryExpressionTask, "AngryFace",
                            4096, NULL, 5, &UIContext::angryTaskHandle, 1) != pdPASS) {
                            LOG(LOG_SYSTEM, "Failed to create AngryFace task");
                            UIContext::isAngryTaskRunning.store(false);
                        }
                    }
                } else {
                    ScanContext::targetFound = false;  // ← Only reset if NOT a target
                }
              }

              // Manufacturer
              if (device->haveManufacturerData()) {
                  std::string mfg = device->getManufacturerData();

                  if (mfg.size() >= 2) {
                      manufacturerId   = (uint8_t)mfg[1] << 8 | (uint8_t)mfg[0];
                  }
                  manufacturerName = getManufacturerName(manufacturerId);

                  LOG(LOG_GATT, devTag + "Manufacturer: " + manufacturerName.c_str());
              }

              // TX Power
              //if (device->haveTXPower()) {
              //    LOG(LOG_GATT, devTag + "TX: " + String(device->getTXPower()));
              //}

              // Appearance
              if (device->haveAppearance()) {
                  LOG(LOG_GATT, devTag + "Appearance: " + String(device->getAppearance()));
              }

              // RAW (immer!)
              auto payload = device->getPayload();
              extractUUIDs(payload);
          }

          String reason;
          if (currentRSSI <= RSSI_IGNORE_THRESHOLD)  reason = "Too far / weak signal";
          else if (currentRSSI <= RSSI_CONNECT_THRESHOLD) reason = "Weak or unstable signal";
          else                                            reason = "Protected (no pairing possible)";

          LOG(LOG_GATT, devTag + reason + ": " + address
              + " (" + String(currentRSSI) + " dBm)");

          // --- Log full device info even without GATT connection ---
          String infoLog = devTag + "Device info (no GATT)\n"
              "   Address:  " + address + "\n"
              "   Name:     " + localName + "\n"
              "   Manuf.:   " + manufacturerName;

          float distance = powf(10.0f,
              (float)(DISTANCE_CONSTANT - currentRSSI) / (float)RSSI_CONSTANT);
          infoLog += "\n   Distance: ~" + String(distance, 2) + " m"
                  + "\n   RSSI:     " + String(currentRSSI) + " dBm";
          LOG(LOG_GATT, infoLog);

          dev.isConnectable = false;
          ExposureResult exposure = analyzeExposure(dev);
          handleExposureResult(exposure, manufacturerName, devTag);

          WebSender::sendDevice(dev, devSessionId, currentRSSI,
                    isIBeacon, isPwnBeaconDevice,
                    dev.hasNotifyData);

          // Sad expression: device visible but connection rejected
          if (!UIContext::isAngryTaskRunning.load() && !UIContext::isSadTaskRunning.load()) {
              if (xTaskCreatePinnedToCore(showSadExpressionTask, "SadFace",
                  4096, NULL, 3, &UIContext::sadTaskHandle, 1) != pdPASS) {
                  LOG(LOG_SYSTEM, "Failed to create SadFace task");
                  UIContext::isSadTaskRunning.store(false);
              }
          }
          delay(1000);
      }

        // --- Wardriving: log device with GPS coordinates if fix is valid ---
        if (NetworkContext::wardrivingEnabled) {
            NetworkContext::gpsManager.update();  // refresh before reading

            if (NetworkContext::gpsManager.isValid()) {
                NetworkContext::wigleLogger.logDevice(
                    address,
                    localName.length() > 0 ? localName : String(dev.name.c_str()),
                    currentRSSI,
                    NetworkContext::gpsManager.getLatitude(),
                    NetworkContext::gpsManager.getLongitude(),
                    NetworkContext::gpsManager.getAltitude(),
                    NetworkContext::gpsManager.getHDOP(),
                    NetworkContext::gpsManager.getTimestamp()
                );
            }
        }

        // --- Clean up NimBLE client ---
        if (pClient != nullptr && pClient->isConnected()) {
            pClient->disconnect();
        }
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;

    }  // end per-device loop

    // =======================================================================
    //  Scan summary
    // =======================================================================
    LOG(LOG_SCAN, "##########################");
    LOG(LOG_SCAN, "Scan summary:");
    LOG(LOG_SCAN, "  Spotted:    " + String(ScanContext::allSpottedDevice.load()));
    LOG(LOG_SCAN, "  Sniffed:    " + String(ScanContext::targetConnects.load()));
    LOG(LOG_SCAN, "  Suspicious: " + String(ScanContext::susDevice.load()));
    LOG(LOG_SCAN, "  Beacons:    " + String(DeviceContext::beaconsFound.load()));
    LOG(LOG_SCAN, "  PwnBeacons: " + String(DeviceContext::pwnbeaconsFound.load()));

    if (ScanContext::highFindingsCount.load()           > 0 ||
        ScanContext::unencryptedSensitiveCount.load()   > 0 ||
        ScanContext::writableNoAuthCount.load()         > 0) {
        LOG(LOG_SCAN, "  --- Security ---");
        LOG(LOG_SCAN, "  HIGH findings:   " + String(ScanContext::highFindingsCount.load()));
        LOG(LOG_SCAN, "  Sensitive unenc: " + String(ScanContext::unencryptedSensitiveCount.load()));
        LOG(LOG_SCAN, "  Writable noAuth: " + String(ScanContext::writableNoAuthCount.load()));
    }

    if (NetworkContext::wardrivingEnabled) {
        LOG(LOG_SCAN, "  WiGLE log:  " + String(NetworkContext::wigleLogger.getLoggedCount()));
        NetworkContext::wigleLogger.flush();
    }
    LOG(LOG_SCAN, "##########################\n");

    // Persist XP to SD card after every scan cycle
    DeviceContext::xpManager.save();

    delay(2000);  // brief cooldown before next cycle

    // Clear any stale speech bubble that may have been skipped due to queuing
    clearSpeechBubble();
    displayName.clear();

    ScanContext::scanIsRunning.store(false);
}
