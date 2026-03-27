#include "genericDumpHandler.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../logger/logger.h"

String GenericDumpHandler::dumpService(NimBLEClient* pClient, const std::string& uuid) {
    String result = "";

    if (!pClient) return result;

    NimBLERemoteService* service = pClient->getService(uuid.c_str());
    if (!service) return result;

    LOG(LOG_GATT, "     Unknown Service (0x" + String(uuid.c_str()) + ")");

    auto characteristics = service->getCharacteristics(true);
    for (auto* pChar : characteristics) {
        std::string charUuid = pChar->getUUID().toString();
        String props = "";
        if (pChar->canRead()) props += "R";
        if (pChar->canWrite()) props += "W";
        if (pChar->canNotify()) props += "N";
        if (pChar->canIndicate()) props += "I";

        String line = "  Char " + String(charUuid.c_str()) + " [" + props + "]";

        if (pChar->canRead()) {
            std::string raw = pChar->readValue();
            if (!raw.empty()) {
                String hex = "";
                size_t maxLen = 64; // UI limit
                size_t len = (raw.size() > maxLen) ? maxLen : raw.size();

                for (size_t i = 0; i < len; i++) {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02X ", (uint8_t)raw[i]);
                    hex += buf;
                }
                if (raw.size() > maxLen) {
                    hex += "...";
                }
                // Optional but VERY useful:
                line += " (len=" + String(raw.size()) + ")";
                line += " = " + hex;
            }
        }

        //line += "\n";
        result += line;
        LOG(LOG_GATT, "     " + line);
    }

    if (result.isEmpty()) {
        result = "Unknown Service (" + String(uuid.c_str()) + "): No characteristics\n";
    }

    return result;
}
