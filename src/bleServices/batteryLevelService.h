#ifndef BATTERY_SERVICE_HANDLER_H
#define BATTERY_SERVICE_HANDLER_H

#include <Arduino.h>
#include <BLEDevice.h>

class BatteryServiceHandler {
public:
    static String readBatteryLevel(BLEDevice peripheral);
};

#endif
