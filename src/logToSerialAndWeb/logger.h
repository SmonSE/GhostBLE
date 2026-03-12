#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;
extern SemaphoreHandle_t logMutex;

void initLogger();
void logToSerialAndWeb(const String& msg);

#endif // LOGGER_H
