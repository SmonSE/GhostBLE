// sus_log_context.cpp
#include "sus_log_context.h"

namespace SusLog {

static Entry buffer[CAPACITY];
static int   writeIdx = 0;
static int   entryCount = 0;

void add(const char* label, const char* mac, int8_t rssi) {
    Entry& e = buffer[writeIdx];
    strncpy(e.label, label, sizeof(e.label) - 1);
    e.label[sizeof(e.label) - 1] = '\0';
    strncpy(e.mac, mac, sizeof(e.mac) - 1);
    e.mac[sizeof(e.mac) - 1] = '\0';
    e.rssi = rssi;
    e.timestamp = millis();

    writeIdx = (writeIdx + 1) % CAPACITY;
    if (entryCount < CAPACITY) entryCount++;
}

int count() { return entryCount; }

const Entry& get(int index) {
    int actualIdx = (writeIdx - 1 - index + CAPACITY) % CAPACITY;
    return buffer[actualIdx];
}

} // namespace SusLog
