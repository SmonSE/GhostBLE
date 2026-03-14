#include "BLEDecoder.h"
#include "../config/config.h"
#include "../logger/logger.h"
#include "../sdCard/SDLogger.h"
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

void decodeBLEData(const std::string& uuid, uint8_t* data, size_t length, SDLogger& sdLogger)
{
    String uuidStr = String(uuid.c_str());

    String hexString   = toHex(data, length);
    String asciiString = toASCII(data, length);

    xpManager.awardXP(15);  // +15 XP: notify data received

    // ---- Pretty Output (Serial/Web) ----
    logToSerialAndWeb("BLE Notify");
    logToSerialAndWeb("   UUID : " + uuidStr);
    logToSerialAndWeb("   HEX  : " + hexString);
    logToSerialAndWeb("   ASCII: " + asciiString);

    // ---- Compact LogLine (SD Card) ----
    String logLine;
    logLine.reserve(200);
    logLine += "BLE Notify: UUID=";
    logLine += uuidStr;
    logLine += " | HEX=";
    logLine += hexString;
    logLine += " | ASCII=";
    logLine += asciiString;

    // ---- Known BLE characteristics ----
    if (uuidStr.endsWith(UUID_BATTERY_LEVEL) && length >= 1)
    {
        xpManager.awardXP(25);  // +25 XP: known char decoded (battery)
        String battery = String(data[0]) + "%";

        logToSerialAndWeb("   Battery Level: " + battery);
        logLine += " | Battery=";
        logLine += battery;
        logLine += "%";
    }

    if (uuidStr == UUID_BATTERY_LEVEL && length == 1) {
        uint8_t battery = data[0];
        Serial.printf("Battery Level: %d%%\n", battery);
        logLine += " | Battery Level=";
        logLine += String(battery);
        logLine += "%";
    }

    if (uuidStr.endsWith(UUID_DEVICE_NAME))
    {
        xpManager.awardXP(25);  // +25 XP: known char decoded (name)
        String name = asciiString;

        logToSerialAndWeb("   Device Name: " + name);
        logLine += " | Name=";
        logLine += name;
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

        logToSerialAndWeb("   UINT16: " + values);
        logLine += " | UINT16=";
        logLine += values;
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

        logToSerialAndWeb("   UINT32: " + values);
        logLine += " | UINT32=";
        logLine += values;
    }

    // ---- FLOAT32 detection ----
    if (length == 4)
    {
        xpManager.awardXP(10);  // +10 XP: FLOAT payload decoded
        float f;
        memcpy(&f, data, 4);

        String value = String(f, 6);

        logToSerialAndWeb("   FLOAT32: " + value);
        logLine += " | FLOAT32=";
        logLine += value;
    }

    logToSerialAndWeb("");

    // ---- Write ONE compact line to SD ----
    sdLogger.writeCategory(logLine);
}