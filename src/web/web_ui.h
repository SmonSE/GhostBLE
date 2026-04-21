#pragma once

#include <Arduino.h>
#include <vector>
#include <string>

// ============================================================
//  WebUI — statische Web-Ressourcen
//
//  index_html  → PROGMEM-String für den WebSocket-Dashboard
//  roomWords   → Raumwort-Liste für Umgebungserkennung
//
//  Keine Zustandsvariablen hier — nur read-only Ressourcen.
// ============================================================

// ------------------------------------------------------------
//  Raumwörter für Umgebungsnamen-Erkennung
//  (z.B. "wohnzimmer", "office" im Gerätenamen)
// ------------------------------------------------------------
extern const std::vector<std::string> roomWords;

// ------------------------------------------------------------
//  WebSocket-Dashboard HTML
//  PROGMEM: liegt im Flash, nicht im RAM — wichtig auf ESP32
// ------------------------------------------------------------
extern const char index_html[] PROGMEM;
