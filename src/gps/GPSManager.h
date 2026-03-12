#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>

enum class GPSSource {
    GROVE,
    LORA_CAP
};

class GPSManager {
public:
    GPSManager();

    void begin(GPSSource source);
    void switchSource(GPSSource source);
    GPSSource getSource() const;
    const char* getSourceName() const;

    void update();

    bool isValid() const;
    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;
    float getHDOP() const;
    uint32_t getSatellites() const;
    String getTimestamp() const;

private:
    TinyGPSPlus gps;
    GPSSource currentSource;
    HardwareSerial gpsSerial;

    void initSerial(GPSSource source);
};
