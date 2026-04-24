#pragma once

#include <Arduino.h>
#include "core/models/device_info.h"
#include "app/context/scan_context.h"
#include "app/context/device_context.h"

// ============================================================
//  WebSender — structured JSON WebSocket events for the Web UI
//
//  All functions are no-ops when WiFi/WebSocket is not active.
//  Call these after updating context state, not before.
//
//  Message types emitted:
//    { "type": "device", "data": { ... } }
//    { "type": "log",    "data": "..."  }
//    { "type": "stats",  "data": { ... } }
//    { "type": "config", "data": { ... } }
// ============================================================

namespace WebSender {

// Send a single device update to all connected WebSocket clients.
// Call once per device after GATT read + exposure analysis.
void sendDevice(const DeviceInfo& dev,
                int               ghostId,
                int               rssi,
                bool              isBeacon    = false,
                bool              isPwnBeacon = false,
                bool              hasNotify   = false);

// Send a log line. Category matches CSS class in the frontend:
// "scan" | "gatt" | "security" | "beacon" | "notify" | "privacy" | "system"
void sendLog(const String& line, const String& category = "system");

// Send current scan stats (spotted / sus / beacons / pwn).
// Call at end of each scan cycle.
void sendStats();

// Send current device config to newly connected client.
void sendConfig();

} // namespace WebSender
