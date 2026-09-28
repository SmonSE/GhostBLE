#include "rssi_tracker.h"
#include <cmath>

// ===========================================================================
//  DeviceState helpers
// ===========================================================================

void RssiTracker::DeviceState::pushSample(int rssi) {
    samples[writeIdx] = rssi;
    writeIdx = (writeIdx + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE) count++;
    lastSeenMs = millis();
}

// Average of the older half of the ring buffer (oldest -> midpoint).
// Only meaningful once count >= 2; caller checks count before using this.
float RssiTracker::DeviceState::averageFirstHalf() const {
    if (count < 2) return 0.0f;
    size_t half = count / 2;
    // Oldest sample is at writeIdx (since it will be overwritten next),
    // walking forward from there gives chronological order.
    size_t start = (writeIdx + HISTORY_SIZE - count) % HISTORY_SIZE;
    float sum = 0.0f;
    for (size_t i = 0; i < half; i++) {
        sum += samples[(start + i) % HISTORY_SIZE];
    }
    return sum / (float)half;
}

float RssiTracker::DeviceState::averageSecondHalf() const {
    if (count < 2) return 0.0f;
    size_t half   = count / 2;
    size_t rest   = count - half;
    size_t start  = (writeIdx + HISTORY_SIZE - count) % HISTORY_SIZE;
    float sum = 0.0f;
    for (size_t i = 0; i < rest; i++) {
        sum += samples[(start + half + i) % HISTORY_SIZE];
    }
    return sum / (float)rest;
}

// ===========================================================================
//  RssiTracker
// ===========================================================================

void RssiTracker::update(const std::string &key, int rawRssi) {
    DeviceState &state = devices_[key];

    state.pushSample(rawRssi);

    if (!state.emaInit) {
        state.ema     = (float)rawRssi;
        state.emaInit = true;
    } else {
        state.ema = EMA_ALPHA * (float)rawRssi + (1.0f - EMA_ALPHA) * state.ema;
    }
}

float RssiTracker::getSmoothedRssi(const std::string &key) const {
    auto it = devices_.find(key);
    if (it == devices_.end() || !it->second.emaInit) return 0.0f;
    return it->second.ema;
}

float RssiTracker::distanceFromRssi(float rssi) const {
    // Log-distance path-loss model:
    //   distance = 10 ^ ((txPowerAt1m - rssi) / (10 * n))
    float exponent = (txPowerAt1m - rssi) / (10.0f * pathLossExponent);
    float distance = powf(10.0f, exponent);

    if (distance < 0.0f || distance > 1000.0f || isnan(distance) || isinf(distance))
        return -1.0f;

    return distance;
}

float RssiTracker::estimateDistance(const std::string &key) const {
    auto it = devices_.find(key);
    if (it == devices_.end() || !it->second.emaInit) return -1.0f;
    return distanceFromRssi(it->second.ema);
}

float RssiTracker::estimateDistance(int rssi) const {
    return distanceFromRssi((float)rssi);
}

bool RssiTracker::isApproaching(const std::string &key) const {
    auto it = devices_.find(key);
    if (it == devices_.end()) return false;

    const DeviceState &state = it->second;
    if (state.count < APPROACH_MIN_SAMPLES) return false;

    float rssiOld = state.averageFirstHalf();
    float rssiNew = state.averageSecondHalf();

    float distOld = distanceFromRssi(rssiOld);
    float distNew = distanceFromRssi(rssiNew);

    if (distOld < 0.0f || distNew < 0.0f) return false;

    // "Approaching" = distance meaningfully decreased, not just RSSI jitter.
    return (distOld - distNew) >= APPROACH_MIN_DELTA_M;
}

size_t RssiTracker::sampleCount(const std::string &key) const {
    auto it = devices_.find(key);
    if (it == devices_.end()) return 0;
    return it->second.count;
}

void RssiTracker::prune() {
    unsigned long now = millis();
    for (auto it = devices_.begin(); it != devices_.end(); ) {
        if (now - it->second.lastSeenMs > STALE_AGE_MS) {
            it = devices_.erase(it);
        } else {
            ++it;
        }
    }
}

void RssiTracker::reset(const std::string &key) {
    devices_.erase(key);
}

void RssiTracker::calibrate(float newTxPowerAt1m, float newPathLossExponent) {
    txPowerAt1m      = newTxPowerAt1m;
    pathLossExponent = newPathLossExponent;
}
