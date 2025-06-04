#ifndef HEART_RATE_SERVICE_H
#define HEART_RATE_SERVICE_H

#include <NimBLEClient.h>
#include <Arduino.h>

class HeartRateServiceHandler {
public:
  static String readHeartRate(NimBLEClient* pClient);
};

#endif
