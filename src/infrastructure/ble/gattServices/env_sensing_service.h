#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>

class EnvironmentalSensingServiceHandler {
public:
    static String readEnvironmentalSensing(NimBLEClient* pClient);
};
