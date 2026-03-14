#include "WigleLogger.h"
#include "../logger/logger.h"

WigleLogger::WigleLogger() : active(false), initialized(false), loggedCount(0) {}

String WigleLogger::generateFilename() {
    // Find next available filename in /GhostBLE/ folder
    for (int i = 1; i <= 9999; i++) {
        char buf[34];
        snprintf(buf, sizeof(buf), "/GhostBLE/wigle_%04d.csv", i);
        if (!SD.exists(buf)) {
            return String(buf);
        }
    }
    return "/GhostBLE/wigle_overflow.csv";
}

void WigleLogger::begin() {
    active = true;
    loggedCount = 0;
    LOG(LOG_SYSTEM, "WigleLogger: Ready (file created on first GPS fix)");
}

bool WigleLogger::openFile() {
    if (initialized) return true;

    filename = generateFilename();
    file = SD.open(filename.c_str(), FILE_WRITE);
    if (!file) {
        LOG(LOG_SYSTEM, "WigleLogger: Failed to create " + filename);
        return false;
    }

    writeHeader();
    initialized = true;
    LOG(LOG_SYSTEM, "WigleLogger: Logging to " + filename);
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
    if (!active) return;

    // Lazily create the file on first device log (only when GPS is valid)
    if (!initialized && !openFile()) return;

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
        LOG(LOG_SYSTEM, "WigleLogger: Closed " + filename + " (" + String(loggedCount) + " entries)");
    }
    initialized = false;
    active = false;
}

String WigleLogger::getFilename() const {
    return initialized ? filename : "(waiting for GPS)";
}

uint32_t WigleLogger::getLoggedCount() const {
    return loggedCount;
}

bool WigleLogger::isReady() const {
    return active;
}
