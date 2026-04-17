#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>

class HeartRateServiceHandler {
public:
  static String readHeartRate(NimBLEClient* pClient);
};
