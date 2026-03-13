#include "logger.h"

AsyncWebSocket ws("/ws");  // Define your AsyncWebSocket object here
SemaphoreHandle_t logMutex = NULL;

void initLogger() {
  logMutex = xSemaphoreCreateMutex();
}

void logToSerialAndWeb(const String& msg) {
  if (logMutex != NULL && xSemaphoreTake(logMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    if (Serial.availableForWrite() > 0) {
      Serial.println(msg);
    }
    if (ws.count() > 0 && ws.availableForWriteAll()) {
      ws.textAll(msg);
    }
    xSemaphoreGive(logMutex);
  } else {
    // Fallback: write to serial without mutex if timeout exceeded
    Serial.println(msg);
  }
}
