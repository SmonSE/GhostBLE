#include "network_context.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "infrastructure/logging/logger.h"
#include "app/context/globals.h"   // deviceConfig, index_html — bis web_ui.h existiert
#include "config/device_config.h"
#include "infrastructure/platform/hardware_config.h"


// ------------------------------------------------------------
//  WebServer + WebSocket — Objekte leben hier
// ------------------------------------------------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ------------------------------------------------------------
//  WebSocket-Event-Handler
//  Fix gegenüber Original: data[len] = '\0' war Out-of-bounds.
//  Stattdessen: String direkt mit Längenangabe konstruieren.
// ------------------------------------------------------------
static void onWsEvent(AsyncWebSocket*       wsServer,
                      AsyncWebSocketClient* client,
                      AwsEventType          type,
                      void*                 arg,
                      uint8_t*              data,
                      size_t                len) {
    if (type == WS_EVT_CONNECT) {
        LOG(LOG_SYSTEM, "WebSocket client connected: " + String(client->id()));

    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 &&
            info->len == len && info->opcode == WS_TEXT) {

            String msg((char*)data, len);   // sicher: kein Null-Terminator-Hack
            String reply = deviceConfig.handleMessage(msg);
            if (reply.length() > 0) {
                client->text(reply);
            }
        }
    }
}

namespace NetworkContext {

// ------------------------------------------------------------
//  WiFi / Web server
// ------------------------------------------------------------
bool isWebLogActive = false;
bool wifiStarted    = false;

// ------------------------------------------------------------
//  Wardriving
// ------------------------------------------------------------
bool wardrivingEnabled = false;

// ------------------------------------------------------------
//  GPS + WiGLE-Logger
//  Objekte leben hier — einzige Instanz im gesamten Projekt.
// ------------------------------------------------------------
GPSManager  gpsManager;
WigleLogger wigleLogger;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------
void startWebServer() {
    if (wifiStarted) return;  // Guard: nicht doppelt starten

    WiFi.mode(WIFI_AP);
    WiFi.softAP(deviceConfig.getWifiSSID().c_str(),
                deviceConfig.getWifiPassword().c_str());

    LOG(LOG_CONTROL, "SoftAP started — IP: " + WiFi.softAPIP().toString());

    // WebSocket-Events und Handler VOR server.begin() registrieren
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", index_html);
    });

    server.begin();
    LOG(LOG_CONTROL, "Web server started.");

    wifiStarted    = true;
    isWebLogActive = true;
    logEnableTarget(TARGET_WEB);
}

void stopWebServer() {
    ws.closeAll();
    server.end();
    delay(50);

    WiFi.softAPdisconnect(true);
    delay(50);
    WiFi.mode(WIFI_OFF);
    delay(100);  // kritisch: ESP32 braucht Zeit für sauberes Shutdown

    wifiStarted    = false;
    isWebLogActive = false;
    logDisableTarget(TARGET_WEB);

    LOG(LOG_CONTROL, "WiFi fully stopped.");
}

bool switchGPSSource() {
    if (!wardrivingEnabled) {
        LOG(LOG_CONTROL, "Enable wardriving first.");
        printf("Enable wardriving first.\n");
        return false;
    }

#if defined(LORA_CS_PIN)
    GPSSource next = (gpsManager.getSource() == GPSSource::GROVE)
                     ? GPSSource::LORA_CAP
                     : GPSSource::GROVE;
    gpsManager.switchSource(next);
    LOG(LOG_CONTROL, "GPS source: " + String(gpsManager.getSourceName()));
#else
    LOG(LOG_CONTROL, "Only Grove GPS available on this device.");
    printf("Only Grove GPS available on this device.\n");
#endif

    return true;
}

} // namespace NetworkContext
