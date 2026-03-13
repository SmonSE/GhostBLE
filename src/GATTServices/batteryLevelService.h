#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class BatteryServiceHandler {
public:
    // Reads battery level from connected client (0x180F service)
    static String readBatteryLevel(NimBLEClient* pClient);
};
