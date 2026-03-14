#include "GPSManager.h"
#include "../config/config.h"
#include "../logger/logger.h"

GPSManager::GPSManager() : currentSource(GPSSource::GROVE), gpsSerial(1) {}

void GPSManager::begin(GPSSource source) {
    currentSource = source;
    initSerial(source);
}

void GPSManager::switchSource(GPSSource source) {
    gpsSerial.end();
    delay(50);
    currentSource = source;
    initSerial(source);
}

GPSSource GPSManager::getSource() const {
    return currentSource;
}

const char* GPSManager::getSourceName() const {
    return (currentSource == GPSSource::GROVE) ? "Grove" : "LoRa";
}

void GPSManager::initSerial(GPSSource source) {
    int rxPin, txPin;

    if (source == GPSSource::GROVE) {
        rxPin = GPS_GROVE_RX;
        txPin = GPS_GROVE_TX;
    } else {
        rxPin = GPS_LORA_RX;
        txPin = GPS_LORA_TX;
    }

    gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
    LOG(LOG_GPS, "GPS: " + String(getSourceName()) + " (RX=" + String(rxPin) + ", TX=" + String(txPin) + ")");
}

void GPSManager::update() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
}

bool GPSManager::isValid() {
    return gps.location.isValid() && gps.location.age() < 5000;
}

double GPSManager::getLatitude() {
    return gps.location.isValid() ? gps.location.lat() : 0.0;
}

double GPSManager::getLongitude() {
    return gps.location.isValid() ? gps.location.lng() : 0.0;
}

double GPSManager::getAltitude() {
    return gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
}

float GPSManager::getHDOP() {
    return gps.hdop.isValid() ? (float)gps.hdop.hdop() : 99.9f;
}

uint32_t GPSManager::getSatellites() {
    return gps.satellites.isValid() ? gps.satellites.value() : 0;
}

String GPSManager::getTimestamp() {
    if (gps.date.isValid() && gps.time.isValid()) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 gps.date.year(), gps.date.month(), gps.date.day(),
                 gps.time.hour(), gps.time.minute(), gps.time.second());
        return String(buf);
    }
    // Fallback: use millis as relative timestamp
    unsigned long sec = millis() / 1000;
    char buf[24];
    snprintf(buf, sizeof(buf), "0000-00-00 %02lu:%02lu:%02lu",
             (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
    return String(buf);
}
