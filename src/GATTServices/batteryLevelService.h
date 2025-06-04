#ifndef BATTERY_LEVEL_SERVICE_H
#define BATTERY_LEVEL_SERVICE_H

#include <NimBLEClient.h>
#include <Arduino.h>

class BatteryServiceHandler {
public:
    // Reads battery level from connected client (0x180F service)
    static String readBatteryLevel(NimBLEClient* pClient);
};

#endif // BATTERY_LEVEL_SERVICE_H
