#include "pwnBeaconService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

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
    NimBLERemoteCharacteristic* faceChr = pwnSvc->getCharacteristic(PWNBEACON_FACE_CHAR_UUID);
    if (faceChr && faceChr->canRead()) {
        info.face = faceChr->readValue().c_str();
        LOG(LOG_BEACON, "   Face:     " + info.face);
    }

    // Read identity JSON
    NimBLERemoteCharacteristic* identChr = pwnSvc->getCharacteristic(PWNBEACON_IDENTITY_CHAR_UUID);
    if (identChr && identChr->canRead()) {
        info.identity = identChr->readValue().c_str();
        LOG(LOG_BEACON, "   Identity: " + info.identity);
    }

    // Read full name (may differ from advertised short name)
    NimBLERemoteCharacteristic* nameChr = pwnSvc->getCharacteristic(PWNBEACON_NAME_CHAR_UUID);
    if (nameChr && nameChr->canRead()) {
        info.gattName = nameChr->readValue().c_str();
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
