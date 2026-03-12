#include "logger.h"

AsyncWebSocket ws("/ws");  // Define your AsyncWebSocket object here
SemaphoreHandle_t logMutex = NULL;

void initLogger() {
  logMutex = xSemaphoreCreateMutex();
}

void logToSerialAndWeb(const String& msg) {
  if (logMutex != NULL && xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    Serial.println(msg);
    if (ws.count() > 0) {
      ws.textAll(msg);
    }
    xSemaphoreGive(logMutex);
  } else {
    // Fallback: at least write to serial if mutex unavailable
    Serial.println(msg);
  }
}
