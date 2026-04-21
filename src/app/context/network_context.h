#pragma once

#include <Arduino.h>
#include <atomic>

#include <ESPAsyncWebServer.h>

#include "infrastructure/gps/gps_manager.h"
#include "infrastructure/wardriving/wigle_logger.h"

// WebServer + WebSocket — definiert in network_context.cpp,
// extern damit GhostBLE.ino und Logger darauf zugreifen können.
extern AsyncWebServer server;
extern AsyncWebSocket ws;

// ============================================================
//  NetworkContext — WiFi/WebSocket, Wardriving, GPS
//
//  Thread-safety:
//    - std::atomic<bool>  → loop() und ScanTask lesen/schreiben
//    - plain bool         → nur aus loop() / setup() verwendet
//    - gpsManager         → nur aus loop() verwendet (update(),
//                           switchSource()) — kein Task-Zugriff
//    - wigleLogger        → nur aus loop() verwendet
// ============================================================

namespace NetworkContext {

// ------------------------------------------------------------
//  WiFi / Web server
// ------------------------------------------------------------
extern bool isWebLogActive;   // WebSocket-Logging aktiv
extern bool wifiStarted;      // SoftAP läuft gerade

// ------------------------------------------------------------
//  Wardriving
//  atomic: ScanTask liest wardrivingEnabled um zu entscheiden
//  ob GPS-Koordinaten geloggt werden sollen.
// ------------------------------------------------------------
extern bool wardrivingEnabled;

// ------------------------------------------------------------
//  GPS  (nur loop() greift zu)
// ------------------------------------------------------------
extern GPSManager gpsManager;

// ------------------------------------------------------------
//  WiGLE-Logger  (nur loop() greift zu)
// ------------------------------------------------------------
extern WigleLogger wigleLogger;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------

// Startet SoftAP + WebSocket-Server.
// Interner Guard verhindert doppelten Start.
void startWebServer();

// Stoppt WebSocket-Clients, Server und SoftAP sauber.
void stopWebServer();

// Wechselt GPS-Quelle (GROVE ↔ LORA_CAP).
// Gibt false zurück wenn Wardriving nicht aktiv ist.
bool switchGPSSource();

} // namespace NetworkContext
