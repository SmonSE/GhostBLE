#include "PwnBeacon.h"
#include "../logToSerialAndWeb/logger.h"
#include <mbedtls/sha256.h>

// --- Server callbacks ---
class PwnBeaconServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    Serial.println("[PwnBeacon] Peer connected via GATT");
    // Resume advertising so other peers can still discover us
    NimBLEDevice::startAdvertising();
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    Serial.println("[PwnBeacon] Peer disconnected");
    NimBLEDevice::startAdvertising();
  }
};

// --- Signal write callback ---
class PwnBeaconSignalCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    std::string value = pChar->getValue();
    Serial.printf("[PwnBeacon] Signal received: %s\n", value.c_str());
  }
};

// --- PwnBeaconManager ---

PwnBeaconManager::PwnBeaconManager()
    : active(false),
      localPwndRun(0),
      localPwndTot(0),
      peerCount(0),
      bleServer(nullptr),
      charIdentity(nullptr),
      charFace(nullptr),
      charName(nullptr),
      charSignal(nullptr) {
  memset(localName, 0, sizeof(localName));
  memset(localIdentity, 0, sizeof(localIdentity));
  strncpy(localFace, "(>_<)", sizeof(localFace) - 1);
  memset(localFingerprint, 0, sizeof(localFingerprint));
  memset(peers, 0, sizeof(peers));
}

void PwnBeaconManager::computeFingerprint(const char* id, uint8_t* out) {
  uint8_t hash[32];
  mbedtls_sha256((const unsigned char*)id, strlen(id), hash, 0);
  memcpy(out, hash, PWNBEACON_FINGERPRINT_LEN);
}

void PwnBeaconManager::buildAdvPayload(uint8_t* buf, size_t* len) {
  pwnbeacon_adv_t adv;
  memset(&adv, 0, sizeof(adv));

  adv.version  = PWNBEACON_PROTOCOL_VERSION;
  adv.flags    = PWNBEACON_FLAG_ADVERTISE | PWNBEACON_FLAG_CONNECTABLE;
  adv.pwnd_run = localPwndRun;
  adv.pwnd_tot = localPwndTot;
  memcpy(adv.fingerprint, localFingerprint, PWNBEACON_FINGERPRINT_LEN);

  size_t nameLen = strlen(localName);
  if (nameLen > PWNBEACON_ADV_MAX_NAME_LEN) {
    nameLen = PWNBEACON_ADV_MAX_NAME_LEN;
  }
  adv.name_len = (uint8_t)nameLen;
  memcpy(adv.name, localName, nameLen);

  *len = offsetof(pwnbeacon_adv_t, name) + nameLen;
  memcpy(buf, &adv, *len);
}

int PwnBeaconManager::findPeerByFingerprint(const uint8_t* fp) {
  for (uint8_t i = 0; i < peerCount; i++) {
    if (memcmp(peers[i].fingerprint, fp, PWNBEACON_FINGERPRINT_LEN) == 0) {
      return i;
    }
  }
  return -1;
}

void PwnBeaconManager::parseAdvPayload(const uint8_t* data, size_t len, int8_t rssiVal) {
  if (len < offsetof(pwnbeacon_adv_t, name)) {
    return;
  }

  pwnbeacon_adv_t adv;
  memset(&adv, 0, sizeof(adv));
  size_t copyLen = len < sizeof(adv) ? len : sizeof(adv);
  memcpy(&adv, data, copyLen);

  if (adv.version != PWNBEACON_PROTOCOL_VERSION) {
    return;
  }

  // Don't add ourselves
  if (memcmp(adv.fingerprint, localFingerprint, PWNBEACON_FINGERPRINT_LEN) == 0) {
    return;
  }

  int idx = findPeerByFingerprint(adv.fingerprint);

  if (idx >= 0) {
    // Update existing peer
    peers[idx].rssi      = rssiVal;
    peers[idx].last_seen = millis();
    peers[idx].gone      = false;
    peers[idx].pwnd_run  = adv.pwnd_run;
    peers[idx].pwnd_tot  = adv.pwnd_tot;
    return;
  }

  if (peerCount >= PWNBEACON_MAX_PEERS) {
    return;
  }

  uint8_t nameLen = adv.name_len;
  if (nameLen > PWNBEACON_ADV_MAX_NAME_LEN) {
    nameLen = PWNBEACON_ADV_MAX_NAME_LEN;
  }

  memset(&peers[peerCount], 0, sizeof(PwnBeaconPeer));
  peers[peerCount].name      = String(adv.name).substring(0, nameLen);
  peers[peerCount].pwnd_run  = adv.pwnd_run;
  peers[peerCount].pwnd_tot  = adv.pwnd_tot;
  peers[peerCount].rssi      = rssiVal;
  peers[peerCount].last_seen = millis();
  peers[peerCount].gone      = false;
  peers[peerCount].full_data = false;
  memcpy(peers[peerCount].fingerprint, adv.fingerprint, PWNBEACON_FINGERPRINT_LEN);

  lastFriendName = peers[peerCount].name;
  peerCount++;

  logToSerialAndWeb("[PwnBeacon] New peer: " + lastFriendName +
                    " (RSSI: " + String(rssiVal) + ", pwnd: " +
                    String(adv.pwnd_run) + "/" + String(adv.pwnd_tot) + ")");
}

String PwnBeaconManager::buildIdentityJson() {
  String json = "{";
  json += "\"pal\":true,";
  json += "\"name\":\"" + String(localName) + "\",";
  json += "\"face\":\"" + String(localFace) + "\",";
  json += "\"epoch\":1,";
  json += "\"grid_version\":\"2.0.0-ble\",";
  json += "\"identity\":\"" + String(localIdentity) + "\",";
  json += "\"pwnd_run\":" + String(localPwndRun) + ",";
  json += "\"pwnd_tot\":" + String(localPwndTot) + ",";
  json += "\"session_id\":\"" + String(NimBLEDevice::getAddress().toString().c_str()) + "\",";
  json += "\"timestamp\":" + String((int)(millis() / 1000)) + ",";
  json += "\"uptime\":" + String((int)(millis() / 1000)) + ",";
  json += "\"version\":\"1.8.4\"";
  json += "}";
  return json;
}

void PwnBeaconManager::begin(const char* name, const char* identity) {
  strncpy(localName, name, PWNBEACON_ADV_MAX_NAME_LEN);
  localName[PWNBEACON_ADV_MAX_NAME_LEN] = '\0';
  strncpy(localIdentity, identity, sizeof(localIdentity) - 1);
  localIdentity[sizeof(localIdentity) - 1] = '\0';
  computeFingerprint(identity, localFingerprint);

  // NimBLE is already initialized by GhostBLE.ino — just create a server
  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new PwnBeaconServerCB());

  NimBLEService* service = bleServer->createService(PWNBEACON_SERVICE_UUID);

  // Identity characteristic — full JSON payload
  charIdentity = service->createCharacteristic(
      PWNBEACON_IDENTITY_CHAR_UUID,
      NIMBLE_PROPERTY::READ);
  charIdentity->setValue(buildIdentityJson());

  // Face characteristic — current mood
  charFace = service->createCharacteristic(
      PWNBEACON_FACE_CHAR_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  charFace->setValue(localFace);

  // Name characteristic
  charName = service->createCharacteristic(
      PWNBEACON_NAME_CHAR_UUID,
      NIMBLE_PROPERTY::READ);
  charName->setValue(localName);

  // Signal characteristic — write-only for pinging
  charSignal = service->createCharacteristic(
      PWNBEACON_SIGNAL_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE);
  charSignal->setCallbacks(new PwnBeaconSignalCB());

  service->start();

  active = true;
  peerCount = 0;
  lastFriendName = "";

  logToSerialAndWeb("[PwnBeacon] Initialized: " + String(name));
  logToSerialAndWeb("[PwnBeacon] Identity fingerprint: " +
                    String(localFingerprint[0], HEX) +
                    String(localFingerprint[1], HEX) +
                    String(localFingerprint[2], HEX));

  // Start advertising immediately
  advertise();
}

void PwnBeaconManager::end() {
  if (!active) return;

  stopAdvertise();

  // Remove the service from the server
  if (bleServer) {
    bleServer->removeService(
        bleServer->getServiceByUUID(PWNBEACON_SERVICE_UUID));
  }

  active = false;
  peerCount = 0;
  logToSerialAndWeb("[PwnBeacon] Stopped");
}

void PwnBeaconManager::advertise() {
  if (!active) return;

  uint8_t advData[sizeof(pwnbeacon_adv_t)];
  size_t advLen = 0;
  buildAdvPayload(advData, &advLen);

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->reset();
  advertising->addServiceUUID(PWNBEACON_SERVICE_UUID);

  // Set service data in scan response
  NimBLEAdvertisementData scanResponse;
  scanResponse.setServiceData(
      NimBLEUUID(PWNBEACON_SERVICE_UUID),
      std::string((char*)advData, advLen));
  advertising->setScanResponseData(scanResponse);

  advertising->start();
}

void PwnBeaconManager::stopAdvertise() {
  NimBLEDevice::getAdvertising()->stop();
}

void PwnBeaconManager::setFace(const char* face) {
  strncpy(localFace, face, sizeof(localFace) - 1);
  localFace[sizeof(localFace) - 1] = '\0';

  if (charFace) {
    charFace->setValue(localFace);
    charFace->notify();
  }
  if (charIdentity) {
    charIdentity->setValue(buildIdentityJson());
  }
}

void PwnBeaconManager::setPwnd(uint16_t run, uint16_t tot) {
  localPwndRun = run;
  localPwndTot = tot;

  if (charIdentity) {
    charIdentity->setValue(buildIdentityJson());
  }
}

bool PwnBeaconManager::parseAdvertisedDevice(const NimBLEAdvertisedDevice* device) {
  if (!active || !device) return false;

  // Check if the device advertises the PwnBeacon service UUID
  if (!device->isAdvertisingService(NimBLEUUID(PWNBEACON_SERVICE_UUID))) {
    return false;
  }

  // Extract service data
  std::string svcData = device->getServiceData(NimBLEUUID(PWNBEACON_SERVICE_UUID));
  if (svcData.empty()) {
    // Even without service data, this is still a PwnBeacon device
    logToSerialAndWeb("[PwnBeacon] Detected peer (no adv data): " +
                      String(device->getAddress().toString().c_str()));
    return true;
  }

  parseAdvPayload((const uint8_t*)svcData.data(), svcData.length(),
                  device->getRSSI());
  return true;
}

void PwnBeaconManager::checkGonePeers() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < peerCount; i++) {
    if (!peers[i].gone && (now - peers[i].last_seen) > PWNBEACON_PEER_TIMEOUT_MS) {
      peers[i].gone = true;
      logToSerialAndWeb("[PwnBeacon] Peer gone: " + peers[i].name);
    }
  }
}

PwnBeaconPeer* PwnBeaconManager::getPeers() {
  return peers;
}

uint8_t PwnBeaconManager::getPeerCount() {
  return peerCount;
}

String PwnBeaconManager::getLastFriendName() {
  return lastFriendName;
}
