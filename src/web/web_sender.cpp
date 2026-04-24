#include "web_sender.h"

#include <ESPAsyncWebServer.h>
#include "app/context/network_context.h"
#include "app/context/device_context.h"
#include "app/context/scan_context.h"

// ws and server are defined in network_context.cpp, declared extern here
extern AsyncWebSocket ws;

// ---------------------------------------------------------------------------
//  Helper: escape a String for safe JSON embedding.
//  Only handles the characters that appear in BLE device names / MACs.
// ---------------------------------------------------------------------------
static String jsonEscape(const String& s) {
    String out;
    out.reserve(s.length() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            default:
                // Drop non-ASCII — device names can contain garbage bytes
                if ((uint8_t)c >= 32 && (uint8_t)c < 128) out += c;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
//  Guard: skip if WebSocket is not active
// ---------------------------------------------------------------------------
static bool wsReady() {
    return NetworkContext::isWebLogActive && ws.count() > 0;
}

namespace WebSender {

// ---------------------------------------------------------------------------
//  sendDevice
// ---------------------------------------------------------------------------
void sendDevice(const DeviceInfo& dev,
                int               ghostId,
                int               rssi,
                bool              isBeacon,
                bool              isPwnBeacon,
                bool              hasNotify)
{
    if (!wsReady()) return;

    // Build name: prefer GATT name → adv name → manufacturer → "Unknown"
    String name = dev.name.c_str();
    if (name.isEmpty()) name = dev.manufacturer.c_str();
    if (name.isEmpty()) name = "Unknown";

    String mac  = dev.mac.c_str();

    // Build compact JSON — no external lib, keep it small for ESP32 RAM
    String json = "{\"type\":\"device\",\"data\":{"
        "\"ghost_id\":\""     + String(ghostId)                      + "\","
        "\"name\":\""         + jsonEscape(name)                     + "\","
        "\"mac\":\""          + jsonEscape(mac)                      + "\","
        "\"manufacturer\":\"" + jsonEscape(dev.manufacturer.c_str()) + "\","
        "\"rssi\":"           + String(rssi)                         + ","
        "\"connectable\":"    + (dev.isConnectable   ? "true" : "false") + ","
        "\"is_beacon\":"      + (isBeacon            ? "true" : "false") + ","
        "\"is_pwnbeacon\":"   + (isPwnBeacon          ? "true" : "false") + ","
        "\"has_notify\":"     + (hasNotify            ? "true" : "false") + ","
        "\"suspicious\":"     + (ScanContext::targetFound ? "true" : "false")
        + "}}";

    ws.textAll(json);
}

// ---------------------------------------------------------------------------
//  sendLog
// ---------------------------------------------------------------------------
void sendLog(const String& line, const String& category) {
    if (!wsReady()) return;

    // Throttle: skip empty or whitespace-only lines
    if (line.isEmpty()) return;

    String json = "{\"type\":\"log\","
                  "\"category\":\"" + jsonEscape(category) + "\","
                  "\"data\":\""     + jsonEscape(line)     + "\"}";

    ws.textAll(json);
}

// ---------------------------------------------------------------------------
//  sendStats
// ---------------------------------------------------------------------------
void sendStats() {
    if (!wsReady()) return;

    String json = "{\"type\":\"stats\",\"data\":{"
        "\"spotted\":"  + String(ScanContext::allSpottedDevice.load()) + ","
        "\"sus\":"      + String(ScanContext::susDevice.load())        + ","
        "\"beacons\":"  + String(DeviceContext::beaconsFound.load())   + ","
        "\"pwn\":"      + String(DeviceContext::pwnbeaconsFound.load())
        + "}}";

    ws.textAll(json);
}

// ---------------------------------------------------------------------------
//  sendConfig — sent to newly connected clients on GET_CONFIG
// ---------------------------------------------------------------------------
void sendConfig() {
    if (!wsReady()) return;

    String json = "{\"type\":\"config\",\"data\":{"
        "\"name\":\"" + jsonEscape(DeviceContext::deviceConfig.getName()) + "\","
        "\"face\":\"" + jsonEscape(DeviceContext::deviceConfig.getFace()) + "\","
        "\"ssid\":\"" + jsonEscape(DeviceContext::deviceConfig.getWifiSSID())
        + "\"}}";

    ws.textAll(json);
}

} // namespace WebSender