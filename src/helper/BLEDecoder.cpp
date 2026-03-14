#include "BLEDecoder.h"
#include "../config/config.h"
#include "../logger/logger.h"
#include "../globals/globals.h"
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

void decodeBLEData(const std::string& uuid, uint8_t* data, size_t length)
{
    String uuidStr = String(uuid.c_str());

    String hexString   = toHex(data, length);
    String asciiString = toASCII(data, length);

    xpManager.awardXP(15);  // +15 XP: notify data received

    LOG(LOG_NOTIFY, "BLE Notify");
    LOG(LOG_NOTIFY, "   UUID : " + uuidStr);
    LOG(LOG_NOTIFY, "   HEX  : " + hexString);
    LOG(LOG_NOTIFY, "   ASCII: " + asciiString);

    // ---- Known BLE characteristics ----
    if (uuidStr.endsWith(UUID_BATTERY_LEVEL) && length >= 1)
    {
        xpManager.awardXP(25);  // +25 XP: known char decoded (battery)
        String battery = String(data[0]) + "%";
        LOG(LOG_NOTIFY, "   Battery Level: " + battery);
    }

    if (uuidStr == UUID_BATTERY_LEVEL && length == 1) {
        uint8_t battery = data[0];
        LOG(LOG_NOTIFY, "Battery Level: " + String(battery) + "%");
    }

    if (uuidStr.endsWith(UUID_DEVICE_NAME))
    {
        xpManager.awardXP(25);  // +25 XP: known char decoded (name)
        LOG(LOG_NOTIFY, "   Device Name: " + asciiString);
    }

    // ---- UINT16 detection ----
    if (length % 2 == 0 && length <= 16)
    {
        xpManager.awardXP(10);  // +10 XP: UINT payload decoded
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
        xpManager.awardXP(10);  // +10 XP: FLOAT payload decoded
        float f;
        memcpy(&f, data, 4);

        String value = String(f, 6);
        LOG(LOG_NOTIFY, "   FLOAT32: " + value);
    }

    LOG(LOG_NOTIFY, "");
}
