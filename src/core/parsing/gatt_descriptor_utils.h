#pragma once

#include <NimBLERemoteCharacteristic.h>
#include <Arduino.h>


// Reads 0x2901 (Characteristic User Description) if the device provides one.
// Works on any characteristic, known or unknown - returns "" if absent.
String getCharacteristicUserDescription(NimBLERemoteCharacteristic* pChar);
