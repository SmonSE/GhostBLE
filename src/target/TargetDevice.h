#ifndef TARGET_DEVICE_H
#define TARGET_DEVICE_H

#include <Arduino.h>

bool isTargetDevice(String name, String address, String serviceUuid, bool hasManuData);

#endif  // TARGET_DEVICE_H
