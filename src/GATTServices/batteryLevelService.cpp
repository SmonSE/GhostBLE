#include "batteryLevelService.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "../globals/globals.h"
#include "../helper/showExpression.h"
#include "../logToSerialAndWeb/logger.h"

String BatteryServiceHandler::readBatteryLevel(NimBLEClient* pClient) {
  String batteryStr = "";

  logToSerialAndWeb("   🔋 Battery Service");

  if (pClient == nullptr) {
    logToSerialAndWeb("     ⚠️ Client not available");
    return batteryStr;
  }

  // Standard BLE Battery Service (0x180F)
  NimBLERemoteService* batteryService = pClient->getService("180F");
  if (!batteryService) {
    logToSerialAndWeb("     ❌ Battery Service not supported");
    return batteryStr;
  }

  logToSerialAndWeb("     ✅ Battery Service detected (Standard BLE)");

  // Battery Level Characteristic (0x2A19)
  NimBLERemoteCharacteristic* pChar = batteryService->getCharacteristic("2A19");
  if (!pChar) {
    logToSerialAndWeb("     ❌ Battery Level characteristic missing");
    return batteryStr;
  }

  if (!pChar->canRead()) {
    logToSerialAndWeb("     ⚠️ Battery Level not readable");
    return batteryStr;
  }

  std::string raw = pChar->readValue();

  if (raw.empty() || raw.size() < 1) {
    logToSerialAndWeb("     ⚠️ Battery read failed (empty response)");
    return batteryStr;
  }

  uint8_t level = static_cast<uint8_t>(raw[0]);

  if (level > 100) {
    logToSerialAndWeb("     ⚠️ Invalid battery value received");
    return batteryStr;
  }

  // ---------- Human readable output ----------
  batteryStr  = "🔋 Battery: " + String(level) + "%\n";
  batteryStr += "   Access: Public (standard BLE)\n";
  batteryStr += "   Control: Read-only\n";

  logToSerialAndWeb(batteryStr);

  return batteryStr;
}
