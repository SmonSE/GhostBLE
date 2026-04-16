#include "BLEDecoder.h"
#include "config/config.h"
#include "logger/logger.h"
#include "globals/globals.h"
#include <string>

static String toHex(uint8_t* data, size_t len)
{
    String hex;
    hex.reserve(len * 3);
    for (size_t i = 0; i < len; i++)
    {
        if (data[i] < 16) hex += "0";
        hex += String(data[i], HEX);
        hex += " ";
    }
    hex.trim();
    hex.toUpperCase();
    return hex;
}

static String toASCII(uint8_t* data, size_t len)
{
    String ascii;
    ascii.reserve(len);

    for (size_t i = 0; i < len; i++)
    {
        if (data[i] >= 32 && data[i] <= 126)
            ascii += (char)data[i];
        else
            ascii += ".";
    }

    return ascii;
}

// Try to decode a 128-bit vendor UUID as ASCII text.
// Many vendors encode their company name directly into the UUID bytes,
// e.g. "65786365-6c70-6f69-6e74-2e636f6d0000" → "excelpoint.com"
static String decodeVendorUUID(const std::string& uuid)
{
    // Only attempt for full 128-bit UUIDs (36 chars with dashes)
    if (uuid.length() != 36) return "";

    // Strip dashes and decode hex pairs
    String stripped;
    for (size_t i = 0; i < uuid.length(); i++) {
        if (uuid[i] != '-') stripped += uuid[i];
    }
    if (stripped.length() != 32) return "";

    String decoded;
    int printable = 0;
    int total = 0;
    for (size_t i = 0; i < stripped.length(); i += 2) {
        char hexPair[3] = { stripped[i], stripped[i + 1], '\0' };
        uint8_t byte = (uint8_t)strtol(hexPair, nullptr, 16);
        total++;
        if (byte >= 32 && byte <= 126) {
            decoded += (char)byte;
            printable++;
        } else if (byte == 0) {
            // trailing nulls are OK
        } else {
            return ""; // non-printable, non-null → not a text UUID
        }
    }

    // At least 6 printable chars to be meaningful
    if (printable >= 6)
        return decoded;
    return "";
}

void decodeBLEData(const std::string& uuid, uint8_t* data, size_t length)
{
    String uuidStr = String(uuid.c_str());

    String hexString   = toHex(data, length);
    String asciiString = toASCII(data, length);

    xpManager.awardXP(1.5);  // +1.5 XP: notify data received

    LOG(LOG_NOTIFY, devTag + "BLE Notify");

    // Show decoded vendor name if the UUID contains ASCII text
    String vendorName = decodeVendorUUID(uuid);
    if (vendorName.length() > 0)
        LOG(LOG_NOTIFY, "   UUID : " + uuidStr + " [" + vendorName + "]");
    else
        LOG(LOG_NOTIFY, "   UUID : " + uuidStr);
    LOG(LOG_NOTIFY, "   HEX  : " + hexString);
    LOG(LOG_NOTIFY, "   ASCII: " + asciiString);

    // ---- Known BLE characteristics ----
    if (uuidStr.endsWith(UUID_BATTERY_LEVEL) && length >= 1)
    {
        xpManager.awardXP(2.5);  // +2.5 XP: known char decoded (battery)
        String battery = String(data[0]) + "%";
        LOG(LOG_NOTIFY, "   Battery Level: " + battery);
    }

    if (uuidStr == UUID_BATTERY_LEVEL && length == 1) {
        uint8_t battery = data[0];
        LOG(LOG_NOTIFY, "Battery Level: " + String(battery) + "%");
    }

    if (uuidStr.endsWith(UUID_DEVICE_NAME))
    {
        xpManager.awardXP(2.5);  // +2.5 XP: known char decoded (name)
        LOG(LOG_NOTIFY, "   Device Name: " + asciiString);
    }

    // ---- UINT16 detection ----
    if (length % 2 == 0 && length <= 16)
    {
        xpManager.awardXP(1.0);  // +1.0 XP: UINT payload decoded
        String values;
        values.reserve(length * 3);

        for (size_t i = 0; i < length; i += 2)
        {
            uint16_t value = data[i] | (data[i + 1] << 8);
            values += String(value) + " ";
        }

        values.trim();
        LOG(LOG_NOTIFY, "   UINT16: " + values);
    }

    // ---- UINT32 detection ----
    if (length % 4 == 0 && length <= 16)
    {
        String values;
        values.reserve(length * 3);

        for (size_t i = 0; i < length; i += 4)
        {
            uint32_t value =
                data[i] |
                (data[i + 1] << 8) |
                (data[i + 2] << 16) |
                (data[i + 3] << 24);

            values += String(value) + " ";
        }

        values.trim();
        LOG(LOG_NOTIFY, "   UINT32: " + values);
    }

    // ---- FLOAT32 detection ----
    if (length == 4)
    {
        xpManager.awardXP(1.0);  // +1.0 XP: FLOAT payload decoded
        float f;
        memcpy(&f, data, 4);

        String value = String(f, 6);
        LOG(LOG_NOTIFY, "   FLOAT32: " + value);
    }

    LOG(LOG_NOTIFY, "");
}
