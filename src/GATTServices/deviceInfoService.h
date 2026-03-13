#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

class DeviceInfoServiceHandler {
public:
    static String readDeviceInfo(NimBLEClient* pClient);  // Pass the connected client
};
