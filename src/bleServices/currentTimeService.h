#ifndef CURRENT_TIME_SERVICE_H
#define CURRENT_TIME_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>

class CurrentTimeServiceHandler {
public:
  static String readCurrentTime(BLEDevice peripheral);
};

#endif
