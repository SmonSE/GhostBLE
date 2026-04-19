#pragma once

#include <Arduino.h>

class NimBLEClient;

class PulseOximeterServiceHandler {
public:
    static String readSpO2(NimBLEClient* pClient);

private:
    static void handleNotification(uint8_t* data, size_t length);
};