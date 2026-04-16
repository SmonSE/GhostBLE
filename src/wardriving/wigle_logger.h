#pragma once

#include <Arduino.h>
#include <SD.h>

class WigleLogger {
public:
    WigleLogger();

    void begin();
    void logDevice(const String& mac, const String& name, int rssi,
                   double lat, double lon, double alt, float hdop,
                   const String& timestamp);
    void flush();
    void end();
    String getFilename() const;
    uint32_t getLoggedCount() const;
    bool isReady() const;

private:
    File file;
    bool active;
    bool initialized;
    String filename;
    uint32_t loggedCount;

    bool openFile();
    void writeHeader();
    String generateFilename();
    static String escapeCSV(const String& value);
};
