#include "pwn_beacon_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>
#include <mbedtls/sha256.h>

#include "logger/logger.h"
#include "config/device_config.h"

// === Server state ===
static NimBLEServer* pServer = nullptr;
static NimBLEService* pwnService = nullptr;
static NimBLECharacteristic* faceChr = nullptr;
static NimBLECharacteristic* identChr = nullptr;
static NimBLECharacteristic* nameChr = nullptr;
static NimBLECharacteristic* signalChr = nullptr;
static NimBLECharacteristic* messageChr = nullptr;
static NimBLEAdvertising* pwnAdvertising = nullptr;

static String advDeviceName;
static uint16_t advPwndRun = 0;
static uint16_t advPwndTot = 0;
static char ghostIdentity[65] = {0};
static uint8_t ghostFingerprint[PWNBEACON_FINGERPRINT_LEN] = {0};

// Compute SHA-256 fingerprint from identity string (first 6 bytes)
static void computeFingerprint(const char* identity, uint8_t* out) {
    uint8_t hash[32];
    mbedtls_sha256((const unsigned char*)identity, strlen(identity), hash, 0);
    memcpy(out, hash, PWNBEACON_FINGERPRINT_LEN);
}

// Build full identity JSON matching PwnBook/Palnagotchi format
static String buildIdentityJson(const String& deviceName, const String& face) {
    return "{\"name\":\"" + deviceName +
           "\",\"face\":\"" + face +
           "\",\"type\":\"GhostBLE\"" +
           ",\"epoch\":1" +
           ",\"grid_version\":\"2.0.0-ble\"" +
           ",\"identity\":\"" + String(ghostIdentity) + "\"" +
           ",\"pwnd_run\":" + String(advPwndRun) +
           ",\"pwnd_tot\":" + String(advPwndTot) +
           ",\"session_id\":\"" + String(NimBLEDevice::getAddress().toString().c_str()) + "\"" +
           ",\"timestamp\":" + String((int)(millis() / 1000)) +
           ",\"uptime\":" + String((int)(millis() / 1000)) +
           ",\"version\":\"1.0.0\"}";
}

// Build the advertisement service data payload
static void buildAdvPayload(uint8_t* buf, size_t* len) {
    uint8_t nameLen = advDeviceName.length();
    if (nameLen > PWNBEACON_ADV_MAX_NAME_LEN) {
        nameLen = PWNBEACON_ADV_MAX_NAME_LEN;
    }

    // version(1) + flags(1) + pwnd_run(2) + pwnd_tot(2) + fingerprint(6) + name_len(1) + name
    *len = 13 + nameLen;

    buf[0] = PWNBEACON_PROTOCOL_VERSION;
    buf[1] = PWNBEACON_FLAG_ADVERTISE | PWNBEACON_FLAG_CONNECTABLE;
    buf[2] = advPwndRun & 0xFF;         // little-endian
    buf[3] = (advPwndRun >> 8) & 0xFF;
    buf[4] = advPwndTot & 0xFF;         // little-endian
    buf[5] = (advPwndTot >> 8) & 0xFF;
    memcpy(&buf[6], ghostFingerprint, PWNBEACON_FINGERPRINT_LEN);
    buf[12] = nameLen;
    memcpy(&buf[13], advDeviceName.c_str(), nameLen);
}

// === BLE Server Callbacks ===

class PwnBeaconServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
        // Restart advertising so other peers can still discover us
        pwnAdvertising->start();
    }
};

class PwnBeaconSignalCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
        // Signal/ping received
        LOG(LOG_BEACON, "👾 PwnBeacon signal received from " +
            String(connInfo.getAddress().toString().c_str()));
    }
};

class PwnBeaconMessageCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
        std::string raw = characteristic->getValue();
        if (raw.length() > 0) {
            LOG(LOG_BEACON, "👾 PwnBeacon message from " +
                String(connInfo.getAddress().toString().c_str()) +
                ": " + String(raw.c_str()));
        }
    }
};

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

    // Read message
    NimBLERemoteCharacteristic* rMsgChr = pwnSvc->getCharacteristic(PWNBEACON_MESSAGE_CHAR_UUID);
    if (rMsgChr && rMsgChr->canRead()) {
        info.message = rMsgChr->readValue().c_str();
        if (info.message.length() > 0) {
            LOG(LOG_BEACON, "   Message:  " + info.message);
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

    // Derive unique identity from BLE MAC address via SHA-256
    std::string mac = NimBLEDevice::getAddress().toString();
    uint8_t hash[32];
    mbedtls_sha256((const unsigned char*)mac.c_str(), mac.length(), hash, 0);
    for (int i = 0; i < 32; i++) {
        snprintf(ghostIdentity + i * 2, 3, "%02x", hash[i]);
    }
    computeFingerprint(ghostIdentity, ghostFingerprint);

    // Create GATT server with callbacks
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new PwnBeaconServerCallbacks());

    // Create PwnBeacon service
    pwnService = pServer->createService(PWNBEACON_SERVICE_UUID);

    // Identity JSON characteristic (readable)
    identChr = pwnService->createCharacteristic(
        PWNBEACON_IDENTITY_CHAR_UUID,
        NIMBLE_PROPERTY::READ
    );
    identChr->setValue(buildIdentityJson(deviceName, face));

    // Face characteristic (readable + notifiable)
    faceChr = pwnService->createCharacteristic(
        PWNBEACON_FACE_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    faceChr->setValue(face.c_str());

    // Name characteristic (readable)
    nameChr = pwnService->createCharacteristic(
        PWNBEACON_NAME_CHAR_UUID,
        NIMBLE_PROPERTY::READ
    );
    nameChr->setValue(deviceName.c_str());

    // Signal characteristic (writable — ping/poke)
    signalChr = pwnService->createCharacteristic(
        PWNBEACON_SIGNAL_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    signalChr->setCallbacks(new PwnBeaconSignalCallbacks());

    // Message characteristic (read/write/notify)
    messageChr = pwnService->createCharacteristic(
        PWNBEACON_MESSAGE_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    messageChr->setCallbacks(new PwnBeaconMessageCallbacks());
    messageChr->setValue("Scanning the ether...");

    pwnService->start();

    // Configure advertisement
    pwnAdvertising = NimBLEDevice::getAdvertising();
    pwnAdvertising->addServiceUUID(NimBLEUUID(PWNBEACON_SERVICE_UUID));
    pwnAdvertising->setMinInterval(0x20);
    pwnAdvertising->setMaxInterval(0x40);

    // Build advertisement payload
    uint8_t advPayload[13 + PWNBEACON_ADV_MAX_NAME_LEN];
    size_t advPayloadLen = 0;
    buildAdvPayload(advPayload, &advPayloadLen);
    // Scan response: 31 bytes max, 128-bit UUID overhead = 18 bytes, leaving 13
    if (advPayloadLen > 13) advPayloadLen = 13;

    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.setName(deviceName.c_str());
    advData.setCompleteServices(NimBLEUUID(PWNBEACON_SERVICE_UUID));

    NimBLEAdvertisementData scanResp;
    scanResp.setServiceData(NimBLEUUID(PWNBEACON_SERVICE_UUID),
                            std::string((char*)advPayload, advPayloadLen));

    pwnAdvertising->setAdvertisementData(advData);
    pwnAdvertising->setScanResponseData(scanResp);
    pwnAdvertising->start();

    LOG(LOG_BEACON, "👾 PwnBeacon advertising started: " + deviceName);
}

void PwnBeaconServiceHandler::updateCounters(uint16_t pwndRun, uint16_t pwndTot) {
    advPwndRun = pwndRun;
    advPwndTot = pwndTot;

    // Update identity JSON with new counters
    if (identChr) {
        identChr->setValue(buildIdentityJson(advDeviceName, faceChr ? faceChr->getValue().c_str() : deviceConfig.getFace().c_str()));
    }

    // Rebuild and restart advertising with updated counters
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();

    uint8_t advPayload[13 + PWNBEACON_ADV_MAX_NAME_LEN];
    size_t advPayloadLen = 0;
    buildAdvPayload(advPayload, &advPayloadLen);
    if (advPayloadLen > 13) advPayloadLen = 13;

    NimBLEAdvertisementData scanResp;
    scanResp.setServiceData(NimBLEUUID(PWNBEACON_SERVICE_UUID),
                            std::string((char*)advPayload, advPayloadLen));
    pAdvertising->setScanResponseData(scanResp);

    pAdvertising->start();
}

void PwnBeaconServiceHandler::updateFace(const String& face) {
    if (faceChr) {
        faceChr->setValue(face.c_str());
        faceChr->notify();
    }
    if (identChr) {
        identChr->setValue(buildIdentityJson(advDeviceName, face));
    }
}

void PwnBeaconServiceHandler::stopAdvertising() {
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->stop();
    LOG(LOG_BEACON, "👾 PwnBeacon advertising stopped");
}
