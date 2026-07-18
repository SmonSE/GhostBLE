// app/context/device_finder.h
#pragma once
#include <Arduino.h>

namespace DeviceFinder {

struct FoundDevice {
    char   name[24];
    char   mac[18];
    int8_t rssi;
};

static constexpr int MAX_FOUND = 15;

void  scan5s();                     // blockierender 5s-Scan, füllt die Liste
int   count();
const FoundDevice& get(int index);

void startFinderFlow();

} // namespace DeviceFinder