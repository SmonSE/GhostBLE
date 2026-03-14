#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class ImmediateAlertServiceHandler {
public:
    static String readImmediateAlert(NimBLEClient* pClient);
};
