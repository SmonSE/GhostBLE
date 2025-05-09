#ifndef DEVICE_INFO_SERVICE_H
#define DEVICE_INFO_SERVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class DeviceInfoServiceHandler {
public:
    static String readDeviceInfo(NimBLEClient* pClient);  // Pass the connected client
};

#endif // DEVICE_INFO_SERVICE_H
