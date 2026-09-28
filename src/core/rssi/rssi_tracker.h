#pragma once

// ===========================================================================
//  RssiTracker
//  ---------------------------------------------------------------------
//  Centralizes everything related to RSSI handling that used to be spread
//  across ble_scanner.cpp:
//    - smoothing (EMA over a small ring buffer, per device)
//    - distance estimation (single canonical log-distance path-loss model)
//    - approach/trend detection (for stalking-pattern alerts, e.g. Find My
//      trackers whose distance is monotonically decreasing over time)
//    - connect/ignore threshold checks
//
//  Devices are keyed by a std::string (MAC address, or the partial public key
//  for Apple Find My trackers, since their MAC rotates but the key doesn't).
//
//  Memory note: uses std::map<std::string, DeviceState>. Call prune() periodically
//  (e.g. once per scan cycle) to evict stale entries and bound memory use on
//  the ESP32.
// ===========================================================================

#include <Arduino.h>  // for millis(), powf() etc. via Arduino core
#include <map>
#include <string>

class RssiTracker {
public:
    // ---- Tunables -------------------------------------------------------
    // Ring buffer size per device: how many recent raw RSSI samples we keep
    // for trend/approach analysis.
    static constexpr size_t   HISTORY_SIZE = 8;

    // EMA smoothing factor (0..1). Higher = reacts faster to change,
    // lower = smoother but laggier. 0.3 is a reasonable starting point.
    static constexpr float    EMA_ALPHA = 0.3f;

    // Log-distance path-loss model constants:
    //   distance = 10 ^ ((TX_POWER_AT_1M - rssi) / (10 * PATH_LOSS_EXPONENT))
    // TX_POWER_AT_1M: expected RSSI at 1 meter (calibrate per device/case).
    // PATH_LOSS_EXPONENT: environment-dependent, typically 2.0 (free space)
    //   to 4.0 (indoors, obstructions). Default 2.7 is a common indoor middle
    //   ground; re-calibrate with known-distance measurements if possible.
    float txPowerAt1m      = -59.0f;
    float pathLossExponent = 2.7f;

    // Note: RSSI_IGNORE_THRESHOLD / RSSI_CONNECT_THRESHOLD are NOT owned
    // here. They live in scan_config.cpp as mutable globals, switched at
    // runtime by applyScanMode() (FOCUSED/BALANCED/AGGRESSIVE). RssiTracker
    // only handles smoothing/distance/trend — threshold policy stays where
    // the scan-mode system already manages it.

    // How many samples must show a consistent downward distance trend
    // before we report "approaching". Keeps single-sample jitter from
    // triggering false stalking alerts.
    static constexpr size_t APPROACH_MIN_SAMPLES = 4;

    // Minimum distance delta (meters) between the average of the first half
    // and second half of the recent history to call it "approaching" rather
    // than noise.
    static constexpr float APPROACH_MIN_DELTA_M = 1.0f;

    // Entries not updated for longer than this are considered stale and
    // removed by prune().
    static constexpr unsigned long STALE_AGE_MS = 5UL * 60UL * 1000UL; // 5 min

    // ---- Public API -------------------------------------------------------

    // Feed a new raw RSSI sample for a device. Updates EMA + ring buffer.
    // Call this once per detection/scan event for that device.
    void update(const std::string &key, int rawRssi);

    // Smoothed (EMA) RSSI for a device. Returns rawRssi passthrough behavior
    // (0) if the device is unknown.
    float getSmoothedRssi(const std::string &key) const;

    // Distance estimate in meters from a smoothed RSSI value for this device.
    // Returns -1.0f if the device is unknown or the estimate is unrealistic.
    float estimateDistance(const std::string &key) const;

    // Stateless variant: distance estimate directly from a raw RSSI value,
    // using the tracker's calibrated model constants. Useful for one-off
    // estimates (e.g. before a device has any history yet).
    float estimateDistance(int rssi) const;

    // True if the device's recent history shows a consistent, non-trivial
    // decrease in distance (i.e. it is getting closer over time). This is
    // the check to use for stalking-pattern alerts (see suspicious.log:
    // repeated Find My tracker with same partial pubkey, closing distance).
    bool isApproaching(const std::string &key) const;

    // How many samples we currently have for this device (0 if unknown).
    size_t sampleCount(const std::string &key) const;

    // Remove devices not seen for longer than STALE_AGE_MS. Call once per
    // scan cycle to bound memory.
    void prune();

    // Forget a specific device (e.g. after it's been reported/handled).
    void reset(const std::string &key);

    // Re-calibrate the distance model, e.g. after a manual measurement
    // session (known device at known distance).
    void calibrate(float newTxPowerAt1m, float newPathLossExponent);

private:
    struct DeviceState {
        int           samples[HISTORY_SIZE] = {0};
        size_t        count      = 0;   // number of valid samples (<= HISTORY_SIZE)
        size_t        writeIdx   = 0;   // next write position (ring)
        float         ema        = 0.0f;
        bool          emaInit    = false;
        unsigned long lastSeenMs = 0;

        void pushSample(int rssi);
        float averageFirstHalf() const;
        float averageSecondHalf() const;
    };

    std::map<std::string, DeviceState> devices_;

    float distanceFromRssi(float rssi) const;
};
