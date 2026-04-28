#include "network_context.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <M5Unified.h>

#include "infrastructure/logging/logger.h"
#include "app/context/globals.h"
#include "config/device_config.h"
#include "infrastructure/platform/hardware_config.h"
#include "app/context/ui_context.h"

#include "web/web_sender.h"


// ------------------------------------------------------------
//  WebServer + WebSocket — Objects here
// ------------------------------------------------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ------------------------------------------------------------
// WebSocket Event Handler
// Fix compared to the original: `data[len] = '\0'` was out-of-bounds.
// Instead: Construct the string directly with a length specification.
// ------------------------------------------------------------
static void onWsEvent(AsyncWebSocket*       wsServer,
                      AsyncWebSocketClient* client,
                      AwsEventType          type,
                      void*                 arg,
                      uint8_t*              data,
                      size_t                len) {

    if (type == WS_EVT_CONNECT) {
        LOG(LOG_SYSTEM, "WebSocket client connected: " + String(client->id()));
        WebSender::sendConfig();
        WebSender::sendStats();

        // Display aus
        M5.Lcd.sleep();
        NetworkContext::displayEnabled = false;

        // Laufende Expression Tasks stoppen
        UIContext::stopExpressionTask(UIContext::isGlassesTaskRunning,  UIContext::glassesTaskHandle);
        UIContext::stopExpressionTask(UIContext::isAngryTaskRunning,    UIContext::angryTaskHandle);
        UIContext::stopExpressionTask(UIContext::isSadTaskRunning,      UIContext::sadTaskHandle);
        UIContext::stopExpressionTask(UIContext::isHappyTaskRunning,    UIContext::happyTaskHandle);

        // Speech bubble Flag zurücksetzen
        UIContext::isSpeechBubbleActive.store(false);
        
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
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
        M5.Display.wakeup();
        NetworkContext::displayEnabled = true;
        delay(100);
    }
}

namespace NetworkContext {

// ------------------------------------------------------------
//  WiFi / Web server
// ------------------------------------------------------------
bool isWebLogActive = false;
bool wifiStarted    = false;
bool displayEnabled  = true;

// ------------------------------------------------------------
//  Wardriving
// ------------------------------------------------------------
bool wardrivingEnabled = false;

// ------------------------------------------------------------
//  GPS + WiGLE-Logger
// Objects live here — the only instance in the entire project.
// ------------------------------------------------------------
GPSManager  gpsManager;
WigleLogger wigleLogger;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------
void startWebServer() {
    if (wifiStarted) return;  // Guard: do not start twice

    String ssid = deviceConfig.getWifiSSID();
    String pass = deviceConfig.getWifiPassword();

    // Guard: empty credentials crashes WiFi.softAP()
    //if (ssid.isEmpty()) ssid = "GhostBLE";
    //if (pass.isEmpty()) pass = "ghostble123";

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str());

    WiFi.mode(WIFI_AP);
    WiFi.softAP(deviceConfig.getWifiSSID().c_str(),
                deviceConfig.getWifiPassword().c_str());

    LOG(LOG_CONTROL, "SoftAP started — IP: " + WiFi.softAPIP().toString());

    // WebSocket-Events and handler for server.begin() registration
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
    delay(100);  // critical: ESP32 needs time for a clean shutdown

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
