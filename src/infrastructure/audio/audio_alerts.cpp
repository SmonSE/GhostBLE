#include "audio_alerts.h"

#include <Arduino.h>
#include <M5Unified.h>
#include <cstddef>
#include <vector>

#include "ui/menu/menu_controller.h"

namespace AudioAlerts {

namespace {

struct Tone {
    uint16_t frequency;
    uint16_t durationMs;
};

// Plays each tone in sequence, blocking between tones (not after the last
// one) — matches the original inline behavior exactly: no trailing wait
// after the final tone, since nothing needs the speaker silent afterward.
void playSequence(const std::vector<Tone>& tones) {
    for (size_t i = 0; i < tones.size(); i++) {
        M5.Speaker.tone(tones[i].frequency, tones[i].durationMs);
        if (i + 1 < tones.size()) {
            while (M5.Speaker.isPlaying()) { delay(5); }
        }
    }
}

bool suspiciousAudioEnabled() {
    auto* ms = MenuController::getState();
    return ms->audioEnabled && ms->audioSuspicious;
}

bool evilModeAudioEnabled() {
    auto* ms = MenuController::getState();
    return ms->audioEnabled && ms->audioEvilMode;
}

bool droneAudioEnabled() {
    auto* ms = MenuController::getState();
    return ms->audioEnabled && ms->audioDrone;
}

}  // namespace

void playFindMyTrackerAlert() {
    if (!suspiciousAudioEnabled()) return;
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    playSequence({ {1200, 150}, {1200, 150} });
}

void playGattTargetAlert() {
    if (!suspiciousAudioEnabled()) return;
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    playSequence({ {1800, 160}, {1400, 180}, {1000, 200} });
}

void playAdvertisementTargetAlert() {
    if (!suspiciousAudioEnabled()) return;
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    playSequence({ {1760, 200}, {1760, 200} });
}

void playMetaGlassesAlert() {
    if (!evilModeAudioEnabled()) return;
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    playSequence({ {523, 100}, {659, 100}, {784, 100}, {1047, 200} });
}

// Fixed-delay variant (200ms between tones, not "wait until finished" —
// preserved exactly as in the original sdo_handlers.cpp code).
void playDroneAlert() {
    if (!droneAudioEnabled()) return;
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    M5.Speaker.tone(2093, 150);  // hoher Ton = Drohne
    delay(200);
    M5.Speaker.tone(1568, 150);
    delay(200);
    M5.Speaker.tone(2093, 150);
}

void playDroneAlertShort() {
    if (!droneAudioEnabled()) return;
    M5.Speaker.setVolume(MenuController::getAlarmVolume());
    M5.Speaker.tone(2093, 150);
}

}  // namespace AudioAlerts

