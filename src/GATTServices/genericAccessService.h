#ifndef GENERIC_ACCESS_SERVICE_H
#define GENERIC_ACCESS_SERVICE_H

#include <NimBLEClient.h>
#include <Arduino.h>

class GenericAccessServiceHandler {
public:
    static String readGenericAccessInfo(NimBLEClient* pClient);
};

#endif
