#ifndef HEART_RATE_SERVICE_H
#define HEART_RATE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>

class HeartRateServiceHandler {
public:
  static String readHeartRate(BLEDevice peripheral);
};

// Callback function for processing notifications
void onHeartRateNotify(BLECharacteristic* characteristic);

#endif
