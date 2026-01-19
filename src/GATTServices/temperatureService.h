#ifndef TEMP_SERVICE_H
#define TEMP_SERVICE_H

#include <Arduino.h>
#include <NimBLEClient.h>

class TemperatureServiceHandler {
public:
    static String readTemperature(NimBLEClient* pClient);
};

#endif