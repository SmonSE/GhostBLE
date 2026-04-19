#pragma once

#include <Arduino.h>

class NimBLEClient;

class LocationNavigationServiceHandler {
public:
    static String readLocation(NimBLEClient* pClient);

private:
    static void handleNotification(uint8_t* data, size_t length);
};