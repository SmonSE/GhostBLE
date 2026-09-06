#include "gatt_descriptor_utils.h"

#include <NimBLERemoteDescriptor.h>


String getCharacteristicUserDescription(NimBLERemoteCharacteristic* pChar) {
    if (!pChar) return "";

    NimBLERemoteDescriptor* pDesc = pChar->getDescriptor(NimBLEUUID((uint16_t)0x2901));
    if (!pDesc) return "";

    std::string raw = pDesc->readValue();
    if (raw.empty()) return "";

    return String(raw.c_str());
}
