#pragma once
#include <Arduino.h>
#include <string>

inline String detectBinaryFormat(const std::string& value) {
    if (value.size() < 2) return "";

    uint8_t b0 = (uint8_t)value[0];
    uint8_t b1 = (uint8_t)value[1];

    if (b0 == 0x1F && b1 == 0x8B) {
        return "GZIP compressed data";
    }
    if (b0 == 0x50 && b1 == 0x4B) {
        return "ZIP/JAR archive";
    }
    if (value.size() >= 4 &&
        (uint8_t)value[0] == 0x89 && (uint8_t)value[1] == 0x50 &&
        (uint8_t)value[2] == 0x4E && (uint8_t)value[3] == 0x47) {
        return "PNG image data";
    }

    return "";
}
