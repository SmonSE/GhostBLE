#include "BLEDecoder.h"
#include "../logToSerialAndWeb/logger.h"
#include "../sdCard/SDLogger.h"
#include <string>

// Forward declarations of required services/classes
extern SDLogger sdLogger;

static String toHex(uint8_t* data, size_t len)
{
    String hex = "";
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
    String ascii = "";

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

    // ---- Pretty Output (Serial/Web) ----
    logToSerialAndWeb("BLE Notify");
    logToSerialAndWeb("   UUID : " + uuidStr);
    logToSerialAndWeb("   HEX  : " + hexString);
    logToSerialAndWeb("   ASCII: " + asciiString);

    // ---- Compact LogLine (SD Card) ----
    String logLine;
    logLine.reserve(200);
    logLine += "BLE Notify: UUID=" + uuidStr;
    logLine += " | HEX=" + hexString;
    logLine += " | ASCII=" + asciiString;

    // ---- Known BLE characteristics ----
    if (uuidStr.endsWith("2a19") && length >= 1)
    {
        String battery = String(data[0]) + "%";

        logToSerialAndWeb("   Battery Level: " + battery);
        logLine += " | Battery=" + battery;
    }

    if (uuidStr == "2a19" && length == 1) {
        uint8_t battery = data[0];
        Serial.printf("Battery Level: %d%%\n", battery);
    }

    if (uuidStr.endsWith("2a00"))
    {
        String name = asciiString;

        logToSerialAndWeb("   Device Name: " + name);
        logLine += " | Name=" + name;
    }

    // ---- UINT16 detection ----
    if (length % 2 == 0 && length <= 16)
    {
        String values = "";

        for (size_t i = 0; i < length; i += 2)
        {
            uint16_t value = data[i] | (data[i + 1] << 8);
            values += String(value) + " ";
        }

        values.trim();

        logToSerialAndWeb("   UINT16: " + values);
        logLine += " | UINT16=" + values;
    }

    // ---- UINT32 detection ----
    if (length % 4 == 0 && length <= 16)
    {
        String values = "";

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
        logLine += " | UINT32=" + values;
    }

    // ---- FLOAT32 detection ----
    if (length == 4)
    {
        float f;
        memcpy(&f, data, 4);

        String value = String(f, 6);

        logToSerialAndWeb("   FLOAT32: " + value);
        logLine += " | FLOAT32=" + value;
    }

    logToSerialAndWeb("");

    // ---- Write ONE compact line to SD ----
    sdLogger.writeCategory(logLine);
    logLine.reserve(200);
}