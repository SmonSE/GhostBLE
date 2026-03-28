#include "genericAttributeService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

String GenericAttributeServiceHandler::readGenericAttribute(NimBLEClient* pClient) {
    String result = "";

    if (!pClient) return result;

    NimBLERemoteService* service = pClient->getService("1801");
    if (!service) return result;

    LOG(LOG_GATT, "     Generic Attribute Service detected (0x1801)");

    // Service Changed Characteristic (0x2A05)
    NimBLERemoteCharacteristic* pChar = service->getCharacteristic("2A05");
    if (pChar) {
        result = "Service Changed: Supported";
        if (pChar->canIndicate()) {
            result += " (indicate)";
        }
        //result += "\n";
        LOG(LOG_GATT, "     " + result);
    }

    return result;
}
