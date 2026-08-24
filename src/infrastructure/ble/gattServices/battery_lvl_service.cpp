#include "battery_lvl_service.h"

#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "app/context/globals.h"
#include "ui/expression/show_expression.h"
#include "infrastructure/logging/logger.h"
#include "utils/string_utils.h"


String BatteryServiceHandler::readBatteryLevel(NimBLEClient* pClient) {
  String batteryStr = "";
  String indent = StringUtils::indentFromTag(devTag);

  LOG(LOG_GATT, indent + "Battery Service");

  if (pClient == nullptr) {
    LOG(LOG_GATT, indent + " Client not available");
    return batteryStr;
  }

  // Standard BLE Battery Service (0x180F)
  NimBLERemoteService* batteryService = pClient->getService("180F");
  if (!batteryService) {
    LOG(LOG_GATT, indent + " Battery Service not supported");
    return batteryStr;
  }

  LOG(LOG_GATT, indent + " Battery Service detected (Standard BLE)");

  // Battery Level Characteristic (0x2A19)
  NimBLERemoteCharacteristic* pChar = batteryService->getCharacteristic("2A19");
  if (!pChar) {
    LOG(LOG_GATT, indent + " Battery Level characteristic missing");
    return batteryStr;
  }

  if (!pChar->canRead()) {
    LOG(LOG_GATT, indent + " Battery Level not readable");
    return batteryStr;
  }

  std::string raw = pChar->readValue();

  if (raw.empty() || raw.size() < 1) {
    LOG(LOG_GATT, indent + " Battery read failed (empty response)");
    return batteryStr;
  }

  uint8_t level = static_cast<uint8_t>(raw[0]);

  if (level > 100) {
    LOG(LOG_GATT, indent + " Invalid battery value received");
    return batteryStr;
  }

  // ---------- Human readable output ----------
  batteryStr  = indent + "  Battery: " + String(level) + "%\n";
  batteryStr += indent + "  Access: Public (standard BLE)\n";
  batteryStr += indent + "  Control: Read-only";

  LOG(LOG_GATT,batteryStr);

  return batteryStr;
}
