#pragma once

#include <Arduino.h>

class NimBLEClient;

class WeightServiceHandler {
public:
    static String readWeight(NimBLEClient* pClient);

private:
    static void handleNotification(uint8_t* data, size_t length);
};