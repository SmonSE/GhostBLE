#ifndef HEART_RATE_SERVICE_H
#define HEART_RATE_SERVICE_H

#include "NimBLEDevice.h"

class HeartRateServiceHandler {
public:
  static String readHeartRate(NimBLEClient* pClient);
};

#endif  // HEART_RATE_SERVICE_H
