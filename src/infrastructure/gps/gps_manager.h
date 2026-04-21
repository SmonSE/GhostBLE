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

    bool isValid();
    double getLatitude();
    double getLongitude();
    double getAltitude();
    float getHDOP();
    uint32_t getSatellites();
    String getTimestamp();

private:
    TinyGPSPlus gps;
    GPSSource currentSource;
    HardwareSerial gpsSerial;

    void initSerial(GPSSource source);
};
