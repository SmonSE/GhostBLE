#include "connected_device_context.h"
#include <M5Unified.h>
#include <vector>
#include <algorithm>
#include <cstring>

namespace ConnectedLog {

static std::vector<Entry> entries_;
static uint16_t loggingHandle_ = 0xFFFF; // BLE_HS_CONN_HANDLE_NONE

void add(const char* label, const char* mac, int8_t rssi, uint16_t conn_handle, uint8_t addrType) {
    Entry e{};
    strncpy(e.label, label, sizeof(e.label) - 1);
    strncpy(e.mac, mac, sizeof(e.mac) - 1);
    e.rssi = rssi;
    e.timestamp = millis();
    e.conn_handle = conn_handle;
    e.isLogging = false;
    e.addrType = addrType;
    entries_.push_back(e);
}

void remove(uint16_t conn_handle) {
    if (conn_handle == loggingHandle_) loggingHandle_ = 0xFFFF;
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [conn_handle](const Entry& e) { return e.conn_handle == conn_handle; }),
        entries_.end()
    );
}

void onConnect(uint16_t conn_handle, const char* mac, const char* label, int8_t rssi) {
    Entry e{};
    strncpy(e.mac, mac, sizeof(e.mac) - 1);
    strncpy(e.label, label, sizeof(e.label) - 1);
    e.rssi = rssi;
    e.timestamp = millis();
    e.conn_handle = conn_handle;
    e.isLogging = false;
    entries_.push_back(e);
}

void onDisconnect(uint16_t conn_handle) {
    if (conn_handle == loggingHandle_) {
        loggingHandle_ = 0xFFFF; // Logging endet automatisch mit der Verbindung
    }
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [conn_handle](const Entry& e) { return e.conn_handle == conn_handle; }),
        entries_.end()
    );
}

int count() { return (int)entries_.size(); }
const Entry& get(int idx) { return entries_[idx]; }

void startLogging(uint16_t conn_handle) {
    loggingHandle_ = conn_handle;
    for (auto& e : entries_) e.isLogging = (e.conn_handle == conn_handle);
}

void stopLogging(uint16_t conn_handle) {
    if (loggingHandle_ == conn_handle) loggingHandle_ = 0xFFFF;
    for (auto& e : entries_) if (e.conn_handle == conn_handle) e.isLogging = false;
}

bool isLoggingActive() { return loggingHandle_ != 0xFFFF; }
uint16_t activeLoggingHandle() { return loggingHandle_; }

} // namespace ConnectedLog
