#include "txPowerService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

String TxPowerServiceHandler::readTxPowerLevel(NimBLEClient* pClient) {
    String txPowerStr = "";

    if (pClient == nullptr) {
        return txPowerStr;
    }

    // Standard BLE Tx Power Service (0x1804)
    NimBLERemoteService* txPowerService = pClient->getService("1804");
    if (!txPowerService) {
        return txPowerStr;
    }

    LOG(LOG_GATT, "     Tx Power Service detected (0x1804)");

    // Tx Power Level Characteristic (0x2A07)
    NimBLERemoteCharacteristic* pChar = txPowerService->getCharacteristic("2A07");
    if (!pChar || !pChar->canRead()) {
        return txPowerStr;
    }

    std::string raw = pChar->readValue();
    if (raw.empty()) {
        return txPowerStr;
    }

    int8_t txPower = static_cast<int8_t>(raw[0]);

    txPowerStr = "Tx Power Level: " + String(txPower) + " dBm\n";
    LOG(LOG_GATT, "     Tx Power Level: " + String(txPower) + " dBm");

    return txPowerStr;
}
