#pragma once

#include <stdint.h>
#include <Arduino.h>

struct GATTFingerprint
{
    uint16_t services = 0;
    uint16_t proprietaryServices = 0;
    uint16_t standardServices = 0;
    uint16_t characteristics = 0;

    uint16_t read = 0;
    uint16_t write = 0;
    uint16_t readWrite = 0;

    uint16_t notify = 0;
    uint16_t indicate = 0;

    void reset();

    String toString() const;
};
