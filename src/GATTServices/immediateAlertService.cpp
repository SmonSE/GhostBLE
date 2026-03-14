#include "immediateAlertService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

String ImmediateAlertServiceHandler::readImmediateAlert(NimBLEClient* pClient) {
    String alertStr = "";

    if (!pClient) return alertStr;

    // Immediate Alert Service (0x1802)
    NimBLERemoteService* alertService = pClient->getService("1802");
    if (!alertService) {
        return alertStr;
    }

    LOG(LOG_GATT,"     Immediate Alert Service detected (0x1802)");

    // Alert Level Characteristic (0x2A06)
    NimBLERemoteCharacteristic* pChar = alertService->getCharacteristic("2A06");
    if (!pChar) {
        return alertStr;
    }

    alertStr = "Immediate Alert: Supported";
    if (pChar->canWrite()) {
        alertStr += " (writable)";
    }
    alertStr += "\n";
    LOG(LOG_GATT,"     " + alertStr);

    return alertStr;
}
