#include "pwnBeaconService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

// === Server state ===
static NimBLEServer* pServer = nullptr;
static NimBLEService* pwnService = nullptr;
static NimBLECharacteristic* faceChr = nullptr;
static NimBLECharacteristic* identChr = nullptr;
static NimBLECharacteristic* nameChr = nullptr;

static String advDeviceName;
static uint16_t advPwndRun = 0;
static uint16_t advPwndTot = 0;

// Generate a stable 6-byte fingerprint from the BLE MAC address
static void generateFingerprint(uint8_t fp[PWNBEACON_FINGERPRINT_LEN]) {
    NimBLEAddress addr = NimBLEDevice::getAddress();
    std::string addrStr = addr.toString();
    // Parse "aa:bb:cc:dd:ee:ff" into 6 bytes
    for (int i = 0; i < PWNBEACON_FINGERPRINT_LEN; i++) {
        fp[i] = (uint8_t)strtoul(addrStr.c_str() + (i * 3), nullptr, 16);
    }
}

// Build the advertisement service data payload
static std::string buildAdvPayload() {
    uint8_t fp[PWNBEACON_FINGERPRINT_LEN];
    generateFingerprint(fp);

    uint8_t nameLen = advDeviceName.length();
    if (nameLen > PWNBEACON_ADV_MAX_NAME_LEN) {
        nameLen = PWNBEACON_ADV_MAX_NAME_LEN;
    }

    // version(1) + flags(1) + pwnd_run(2) + pwnd_tot(2) + fingerprint(6) + name_len(1) + name
    size_t payloadLen = 13 + nameLen;
    uint8_t payload[13 + PWNBEACON_ADV_MAX_NAME_LEN];

    payload[0] = PWNBEACON_PROTOCOL_VERSION;
    payload[1] = 0x00;  // flags
    payload[2] = advPwndRun & 0xFF;         // little-endian
    payload[3] = (advPwndRun >> 8) & 0xFF;
    payload[4] = advPwndTot & 0xFF;         // little-endian
    payload[5] = (advPwndTot >> 8) & 0xFF;
    memcpy(&payload[6], fp, PWNBEACON_FINGERPRINT_LEN);
    payload[12] = nameLen;
    memcpy(&payload[13], advDeviceName.c_str(), nameLen);

    return std::string((char*)payload, payloadLen);
}

// === Client (scanning) ===

PwnBeaconInfo PwnBeaconServiceHandler::parseAdvertisement(const uint8_t* data, size_t len) {
    PwnBeaconInfo info;

    // Minimum: version(1) + flags(1) + pwnd_run(2) + pwnd_tot(2) + fingerprint(6) + name_len(1) = 13
    if (len < 13) return info;
    if (data[0] != PWNBEACON_PROTOCOL_VERSION) return info;

    info.version  = data[0];
    info.flags    = data[1];
    info.pwnd_run = data[2] | (data[3] << 8);  // little-endian
    info.pwnd_tot = data[4] | (data[5] << 8);  // little-endian
    memcpy(info.fingerprint, &data[6], PWNBEACON_FINGERPRINT_LEN);

    uint8_t name_len = data[12];
    if (name_len > PWNBEACON_ADV_MAX_NAME_LEN) {
        name_len = PWNBEACON_ADV_MAX_NAME_LEN;
    }
    if (len >= (size_t)(13 + name_len)) {
        info.name = String((const char*)&data[13]).substring(0, name_len);
    }

    info.valid = true;
    return info;
}

void PwnBeaconServiceHandler::readGATT(NimBLEClient* pClient, PwnBeaconInfo& info) {
    if (!pClient) return;

    NimBLERemoteService* pwnSvc = pClient->getService(PWNBEACON_SERVICE_UUID);
    if (!pwnSvc) return;

    // Read face
    NimBLERemoteCharacteristic* rFaceChr = pwnSvc->getCharacteristic(PWNBEACON_FACE_CHAR_UUID);
    if (rFaceChr && rFaceChr->canRead()) {
        info.face = rFaceChr->readValue().c_str();
        LOG(LOG_BEACON, "   Face:     " + info.face);
    }

    // Read identity JSON
    NimBLERemoteCharacteristic* rIdentChr = pwnSvc->getCharacteristic(PWNBEACON_IDENTITY_CHAR_UUID);
    if (rIdentChr && rIdentChr->canRead()) {
        info.identity = rIdentChr->readValue().c_str();
        LOG(LOG_BEACON, "   Identity: " + info.identity);
    }

    // Read full name (may differ from advertised short name)
    NimBLERemoteCharacteristic* rNameChr = pwnSvc->getCharacteristic(PWNBEACON_NAME_CHAR_UUID);
    if (rNameChr && rNameChr->canRead()) {
        info.gattName = rNameChr->readValue().c_str();
        if (info.gattName.length() > 0) {
            LOG(LOG_BEACON, "   Name:     " + info.gattName);
        }
    }
}

String PwnBeaconServiceHandler::fingerprintToString(const uint8_t fingerprint[PWNBEACON_FINGERPRINT_LEN]) {
    char fpStr[18];
    snprintf(fpStr, sizeof(fpStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             fingerprint[0], fingerprint[1],
             fingerprint[2], fingerprint[3],
             fingerprint[4], fingerprint[5]);
    return String(fpStr);
}

// === Server (advertising) ===

void PwnBeaconServiceHandler::startAdvertising(const String& deviceName, const String& face) {
    advDeviceName = deviceName;

    // Create GATT server
    pServer = NimBLEDevice::createServer();

    // Create PwnBeacon service
    pwnService = pServer->createService(PWNBEACON_SERVICE_UUID);

    // Face characteristic (readable)
    faceChr = pwnService->createCharacteristic(
        PWNBEACON_FACE_CHAR_UUID,
        NIMBLE_PROPERTY::READ
    );
    faceChr->setValue(face.c_str());

    // Identity JSON characteristic (readable)
    identChr = pwnService->createCharacteristic(
        PWNBEACON_IDENTITY_CHAR_UUID,
        NIMBLE_PROPERTY::READ
    );
    String identity = "{\"name\":\"" + deviceName + "\",\"type\":\"GhostBLE\"}";
    identChr->setValue(identity.c_str());

    // Name characteristic (readable)
    nameChr = pwnService->createCharacteristic(
        PWNBEACON_NAME_CHAR_UUID,
        NIMBLE_PROPERTY::READ
    );
    nameChr->setValue(deviceName.c_str());

    pwnService->start();

    // Configure advertisement
    // NimBLE builds main adv from addServiceUUID (flags + UUID)
    // We only set scan response manually for the service data payload
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(NimBLEUUID(PWNBEACON_SERVICE_UUID));
    pAdvertising->setName(deviceName.c_str());

    // Scan response: service data with PwnBeacon payload
    std::string payload = buildAdvPayload();
    if (payload.length() > 10) payload.resize(10);

    NimBLEAdvertisementData scanResp;
    scanResp.setServiceData(NimBLEUUID(PWNBEACON_SERVICE_UUID), payload);
    pAdvertising->setScanResponseData(scanResp);

    pAdvertising->start();

    LOG(LOG_BEACON, "👾 PwnBeacon advertising started: " + deviceName);
}

void PwnBeaconServiceHandler::updateCounters(uint16_t pwndRun, uint16_t pwndTot) {
    advPwndRun = pwndRun;
    advPwndTot = pwndTot;

    // Restart advertising with updated counters
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();

    // Update scan response with new payload
    std::string payload = buildAdvPayload();
    if (payload.length() > 10) payload.resize(10);

    NimBLEAdvertisementData scanResp;
    scanResp.setServiceData(NimBLEUUID(PWNBEACON_SERVICE_UUID), payload);
    pAdvertising->setScanResponseData(scanResp);

    pAdvertising->start();
}

void PwnBeaconServiceHandler::stopAdvertising() {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();
    LOG(LOG_BEACON, "👾 PwnBeacon advertising stopped");
}
