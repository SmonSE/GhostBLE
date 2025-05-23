#include "logger.h"

AsyncWebSocket ws("/ws");  // Define your AsyncWebSocket object here


void logToSerialAndWeb(const String& msg) {
  Serial.println(msg);
  if (ws.count() > 0) {
    ws.textAll(msg);
  }
}
