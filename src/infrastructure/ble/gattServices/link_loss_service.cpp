#include "link_loss_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"

String LinkLossServiceHandler::readLinkLoss(NimBLEClient* pClient) {
    String lossStr = "";

    if (!pClient) return lossStr;

    // Link Loss Service (0x1803)
    NimBLERemoteService* lossService = pClient->getService("1803");
    if (!lossService) {
        return lossStr;
    }

    LOG(LOG_GATT,"     Link Loss Service detected (0x1803)");

    // Alert Level Characteristic (0x2A06)
    NimBLERemoteCharacteristic* pChar = lossService->getCharacteristic("2A06");
    if (!pChar) {
        return lossStr;
    }

    if (pChar->canRead()) {
        std::string raw = pChar->readValue();
        if (!raw.empty()) {
            uint8_t level = raw[0];
            const char* levels[] = {"No Alert", "Mild Alert", "High Alert"};
            const char* levelStr = (level <= 2) ? levels[level] : "Unknown";
            lossStr = "Link Loss Alert Level: " + String(levelStr) + "\n";
            LOG(LOG_GATT,"     Link Loss Alert Level: " + String(levelStr));
        }
    }

    return lossStr;
}
