// sus_log_context.h
#pragma once
#include <Arduino.h>

namespace SusLog {

struct Entry {
    char     label[24];
    char     mac[18];
    int8_t   rssi;
    uint32_t timestamp;
};

static constexpr int CAPACITY = 20;

void add(const char* label, const char* mac, int8_t rssi);
int  count();
const Entry& get(int index);

} // namespace SusLog
