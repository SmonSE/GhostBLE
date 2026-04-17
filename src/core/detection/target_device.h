#pragma once

#include <Arduino.h>

bool isTargetDevice(String name, String address, String serviceUuid, String deviceInfoService);

// Check if a BLE device name matches Tesla vehicle naming patterns.
// Legacy format: S + 16 hex chars + [C/D/P/R] (e.g. "Sc155040258896e2dC")
// New format: "Tesla " + 6 chars (e.g. "Tesla 130307")
bool isTeslaDevice(const String& name, const String& serviceUuid);
