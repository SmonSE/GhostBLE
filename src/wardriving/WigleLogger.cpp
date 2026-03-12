#include "WigleLogger.h"

WigleLogger::WigleLogger() : initialized(false), loggedCount(0) {}

String WigleLogger::generateFilename() {
    // Find next available filename: /wigle_0001.csv, /wigle_0002.csv, ...
    for (int i = 1; i <= 9999; i++) {
        char buf[24];
        snprintf(buf, sizeof(buf), "/wigle_%04d.csv", i);
        if (!SD.exists(buf)) {
            return String(buf);
        }
    }
    return "/wigle_overflow.csv";
}

bool WigleLogger::begin() {
    if (initialized) return true;

    filename = generateFilename();
    file = SD.open(filename.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("WigleLogger: Failed to create " + filename);
        return false;
    }

    writeHeader();
    initialized = true;
    loggedCount = 0;
    Serial.println("WigleLogger: Logging to " + filename);
    return true;
}

void WigleLogger::writeHeader() {
    // WiGLE CSV v1.6 header
    file.println("WigleWifi-1.6,appRelease=GhostBLE,model=M5Cardputer,release=1.0,device=ESP32-S3,display=none,board=ESP32-S3,brand=M5Stack");
    file.println("MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type");
    file.flush();
}

String WigleLogger::escapeCSV(const String& value) {
    // If value contains comma, quote, or newline, wrap in quotes
    if (value.indexOf(',') >= 0 || value.indexOf('"') >= 0 || value.indexOf('\n') >= 0) {
        String escaped = value;
        escaped.replace("\"", "\"\"");
        return "\"" + escaped + "\"";
    }
    return value;
}

void WigleLogger::logDevice(const String& mac, const String& name, int rssi,
                            double lat, double lon, double alt, float hdop,
                            const String& timestamp) {
    if (!initialized || !file) return;

    // Estimate accuracy from HDOP (rough: HDOP * 5 meters)
    float accuracy = hdop * 5.0f;
    if (accuracy > 999.0f) accuracy = 999.0f;

    // MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,Lat,Lon,Alt,Accuracy,Type
    String line;
    line.reserve(200);
    line += mac;
    line += ",";
    line += escapeCSV(name.length() > 0 ? name : "<unknown>");
    line += ",[BLE],";
    line += timestamp;
    line += ",0,";
    line += String(rssi);
    line += ",";
    line += String(lat, 6);
    line += ",";
    line += String(lon, 6);
    line += ",";
    line += String(alt, 1);
    line += ",";
    line += String(accuracy, 1);
    line += ",BLE";

    file.println(line);
    loggedCount++;

    // Flush every 10 entries to balance performance and safety
    if (loggedCount % 10 == 0) {
        file.flush();
    }
}

void WigleLogger::flush() {
    if (initialized && file) {
        file.flush();
    }
}

void WigleLogger::end() {
    if (initialized && file) {
        file.flush();
        file.close();
        initialized = false;
        Serial.println("WigleLogger: Closed " + filename + " (" + String(loggedCount) + " entries)");
    }
}

String WigleLogger::getFilename() const {
    return filename;
}

uint32_t WigleLogger::getLoggedCount() const {
    return loggedCount;
}
