#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>

class GenericAttributeServiceHandler {
public:
    static String readGenericAttribute(NimBLEClient* pClient);
};
