#pragma once

#include <Arduino.h>
#include <atomic>

#include <ESPAsyncWebServer.h>

#include "infrastructure/gps/gps_manager.h"
#include "infrastructure/wardriving/wigle_logger.h"

// WebServer + WebSocket — defined in network_context.cpp,
// extern that GhostBLE.ino and Logger be able to access it.
extern AsyncWebServer server;
extern AsyncWebSocket ws;

// ============================================================
//  NetworkContext — WiFi/WebSocket, Wardriving, GPS
//
//  Thread-safety:
//    - std::atomic<bool>  → loop() and ScanTask read/write
//    - plain bool         → only called from loop() / setup(),
//    - gpsManager         → only called from loop() (update(),
//                           switchSource()) — no task access
//    - wigleLogger        → only called from loop().
// ============================================================

namespace NetworkContext {

// ------------------------------------------------------------
//  WiFi / Web server
// ------------------------------------------------------------
extern bool isWebLogActive;   // WebSocket-Logging aktiv
extern bool wifiStarted;      // SoftAP läuft gerade

// ------------------------------------------------------------
//  Wardriving
//  atomic: ScanTask read wardrivingEnabled for decission
//  if GPS-Koordinates should be logged.
// ------------------------------------------------------------
extern bool wardrivingEnabled;

// ------------------------------------------------------------
//  GPS  (only loop() have access)
// ------------------------------------------------------------
extern GPSManager gpsManager;

// ------------------------------------------------------------
//  WiGLE-Logger  (only loop() have access)
// ------------------------------------------------------------
extern WigleLogger wigleLogger;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------

// Starts SoftAP + WebSocket-Server.
// Internal Guard prevent double start.
void startWebServer();

// Stopps WebSocket-Clients, Server and SoftAP clean.
void stopWebServer();

// Change GPS-Source (GROVE ↔ LORA_CAP).
// Return false if Wardriving isn't active.
bool switchGPSSource();

} // namespace NetworkContext
