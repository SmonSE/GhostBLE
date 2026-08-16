#include "unknown_gatt_handler.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "infrastructure/logging/logger.h"
#include "core/parsing/binary_format_detector.h"


bool UnknownGATTHandler::isLikelyUtf8Text(const std::string& data) {
    if (data.empty()) {
        return false;
    }

    size_t printable = 0;
    size_t i = 0;

    while (i < data.size()) {
        uint8_t c = static_cast<uint8_t>(data[i]);

        // ASCII
        if (c >= 0x20 && c <= 0x7E) {
            printable++;
            i++;
            continue;
        }

        // Allow common whitespace
        if (c == '\r' || c == '\n' || c == '\t') {
            i++;
            continue;
        }

        // UTF-8 2-byte sequence
        if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 >= data.size()) return false;

            uint8_t c1 = static_cast<uint8_t>(data[i + 1]);

            if ((c1 & 0xC0) != 0x80) {
                return false;
            }

            printable += 2;
            i += 2;
            continue;
        }

        // UTF-8 3-byte sequence
        if (c >= 0xE0 && c <= 0xEF) {
            if (i + 2 >= data.size()) return false;

            uint8_t c1 = static_cast<uint8_t>(data[i + 1]);
            uint8_t c2 = static_cast<uint8_t>(data[i + 2]);

            if ((c1 & 0xC0) != 0x80 ||
                (c2 & 0xC0) != 0x80) {
                return false;
            }

            printable += 3;
            i += 3;
            continue;
        }

        // UTF-8 4-byte sequence
        if (c >= 0xF0 && c <= 0xF4) {
            if (i + 3 >= data.size()) return false;

            uint8_t c1 = static_cast<uint8_t>(data[i + 1]);
            uint8_t c2 = static_cast<uint8_t>(data[i + 2]);
            uint8_t c3 = static_cast<uint8_t>(data[i + 3]);

            if ((c1 & 0xC0) != 0x80 ||
                (c2 & 0xC0) != 0x80 ||
                (c3 & 0xC0) != 0x80) {
                return false;
            }

            printable += 4;
            i += 4;
            continue;
        }

        return false;
    }

    // Avoid interpreting tiny/binary values as text
    return printable >= 3;
}

String UnknownGATTHandler::getUtf8Text(const std::string& data) {
    size_t end = data.size();

    while (end > 0 && data[end - 1] == '\0') {
        end--;
    }

    if (end == 0) {
        return "";
    }

    std::string text = data.substr(0, end);

    if (!isLikelyUtf8Text(text)) {
        return "";
    }

    return String(text.c_str());
}

String UnknownGATTHandler::dumpService(NimBLEClient* pClient, const std::string& uuid) {
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
                String binaryFormat = detectBinaryFormat(raw);

                line += " (len=" + String(raw.size()) + ")";

                if (!binaryFormat.isEmpty()) {
                    line += " = [" + binaryFormat + "]";
                } else {
                    String hex = "";
                    size_t maxLen = 200;
                    size_t len = (raw.size() > maxLen) ? maxLen : raw.size();

                    for (size_t i = 0; i < len; i++) {
                        char buf[4];
                        snprintf(buf, sizeof(buf), "%02X ", (uint8_t)raw[i]);
                        hex += buf;
                    }

                    if (raw.size() > maxLen) {
                        hex += "...";
                    }

                    line += " = " + hex;

                    // Try to interpret the value as UTF-8 text
                    String text = getUtf8Text(raw);

                    if (!text.isEmpty()) {
                        line += "\n       Text: \"" + text + "\"";
                    }
                }
            }
        }

        result += line;
        LOG(LOG_GATT, "     " + line);
    }

    if (result.isEmpty()) {
        result = "Unknown Service (" + String(uuid.c_str()) + "): No characteristics\n";
    }

    return result;
}
