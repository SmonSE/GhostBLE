#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>

class HIDServiceHandler {
public:
    static String readHID(NimBLEClient* pClient);
};
