#ifndef HEART_RATE_SERVICE_H
#define HEART_RATE_SERVICE_H

#include <Arduino.h>
#include <ArduinoBLE.h>

class HeartRateServiceHandler {
public:
  static String readHeartRate(BLEDevice peripheral);  // ✅ Return a String instead of void
};

#endif
