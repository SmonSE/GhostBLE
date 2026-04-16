#pragma once

#include <Arduino.h>
#include <NimBLEClient.h>

class TemperatureServiceHandler {
public:
    static String readTemperature(NimBLEClient* pClient);
};
