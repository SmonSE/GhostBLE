#ifndef DEVICE_INFO_SERVICE_H
#define DEVICE_INFO_SERVICE_H

#include <Arduino.h>
#include <ArduinoBLE.h>

class DeviceInfoServiceHandler {
public:
  static String readDeviceInfo(BLEDevice peripheral);
};

#endif
