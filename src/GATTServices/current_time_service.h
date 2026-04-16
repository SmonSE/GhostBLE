#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class CurrentTimeServiceHandler {
public:
    static String readCurrentTime(NimBLEClient* pClient);
};
