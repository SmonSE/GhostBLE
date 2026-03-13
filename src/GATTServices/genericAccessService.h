#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class GenericAccessServiceHandler {
public:
    static String readGenericAccessInfo(NimBLEClient* pClient);
};
