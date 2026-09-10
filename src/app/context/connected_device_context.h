#pragma once
#include <cstdint>

namespace ConnectedLog {

struct Entry {
    char mac[18];
    char label[32];
    int8_t rssi;
    uint32_t timestamp;
    uint16_t conn_handle;
    bool isLogging;
    uint8_t addrType;   // BLE_ADDR_PUBLIC oder BLE_ADDR_RANDOM
};

void add(const char* label, const char* mac, int8_t rssi, uint16_t conn_handle, uint8_t addrType);
void remove(uint16_t conn_handle);

int count();
const Entry& get(int idx);

void startLogging(uint16_t conn_handle);
void stopLogging(uint16_t conn_handle);
bool isLoggingActive();
uint16_t activeLoggingHandle();

} // namespace ConnectedLog
