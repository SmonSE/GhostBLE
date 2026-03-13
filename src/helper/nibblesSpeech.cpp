#include "nibblesSpeech.h"
#include <M5Cardputer.h>
#include <M5Unified.h>
#include "drawOverlay.h"
#include "showExpression.h"
#include "../globals/globals.h"
#include "../images/nibblesBubble.h"
#include "../images/nibblesFront.h"
#include "../images/nibblesHappy.h"

// --- Message pools ---

static const char* idleMessages[] = {
    "Hmm...",
    "Anyone here?",
    "Still scanning",
    "Listening...",
    "Where is everyone",
    "So quiet...",
    "Zzz...",
    "Hello?",
    "*yawn*",
    "Bored..."
};
static const int idleMessageCount = sizeof(idleMessages) / sizeof(idleMessages[0]);

static const char* scanStartMessages[] = {
    "Lets go!",
    "Start scanning!",
    "Ready?",
    "Lets begin!",
    "Eyes open!",
    "Here we go!"
};
static const int scanStartMessageCount = sizeof(scanStartMessages) / sizeof(scanStartMessages[0]);

static const char* wardrivingMessages[] = {
    "Lets walk!",
    "BLE hunting!",
    "Lets explore!",
    "On the move!",
    "Scanning area",
    "Adventure time!"
};
static const int wardrivingMessageCount = sizeof(wardrivingMessages) / sizeof(wardrivingMessages[0]);

static const char* suspiciousMessages[] = {
    "$&%#!",
    "Thats weird...",
    "Suspicious...",
    "What is that?!",
    "Hmmmm...",
    "Watch out!",
    "Alert!"
};
static const int suspiciousMessageCount = sizeof(suspiciousMessages) / sizeof(suspiciousMessages[0]);

static const char* levelUpMessages[] = {
    "Level up!",
    "I am learning!",
    "More signals!",
    "Getting better!",
    "Leveled up!",
    "Power up!"
};
static const int levelUpMessageCount = sizeof(levelUpMessages) / sizeof(levelUpMessages[0]);

// --- Timing ---

static unsigned long lastEventTime = 0;
static unsigned long lastSpeechTime = 0;

static const unsigned long IDLE_TIMEOUT_MS = 15000;     // 15 seconds before idle speech
static const unsigned long SPEECH_COOLDOWN_MS = 10000;   // 10 seconds between any speech
static const unsigned long THOUGHT_DISPLAY_MS = 3000;    // 3 seconds display time

static bool thoughtVisible = false;
static unsigned long thoughtShownAt = 0;

// --- Helpers ---

static const char* pickRandom(const char** pool, int count) {
    return pool[random(0, count)];
}

void nibblesSpeechBegin() {
    unsigned long now = millis();
    lastEventTime = now;
    lastSpeechTime = 0;
    randomSeed(analogRead(0) ^ millis());
}

void drawThoughtBubble(const char* message, int x0, int y0) {
    int textLen = strlen(message);
    // Bubble width adapts to text (min 60, max 108 to fit screen)
    int bubbleW = min(108, max(60, textLen * 6 + 16));
    int bubbleH = 22;

    // Draw rounded rect bubble with green theme
    M5.Lcd.fillRoundRect(x0, y0, bubbleW, bubbleH, 4, 0x2444);  // dark green fill
    M5.Lcd.drawRoundRect(x0, y0, bubbleW, bubbleH, 4, 0x07E0);  // green border

    // Small triangle pointer toward NibBLEs (bottom-left)
    int triX = x0 + 8;
    int triY = y0 + bubbleH;
    M5.Lcd.fillTriangle(triX, triY - 1, triX + 6, triY - 1, triX, triY + 4, 0x2444);
    M5.Lcd.drawLine(triX, triY - 1, triX, triY + 4, 0x07E0);
    M5.Lcd.drawLine(triX, triY + 4, triX + 6, triY - 1, 0x07E0);

    // Draw text
    M5.Lcd.setTextColor(0x07E0);  // green text
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x0 + 6, y0 + 7);
    M5.Lcd.print(message);
}

static void clearThoughtBubble() {
    // Redraw the character area to clear the bubble region
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);
}

static void showMumble(const char* message) {
    // Redraw sprite area to clear any previous bubble cleanly
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);

    drawThoughtBubble(message, 125, 18);
    thoughtVisible = true;
    thoughtShownAt = millis();
    lastSpeechTime = millis();
}

void nibblesSpeechNotifyEvent() {
    lastEventTime = millis();

    // If a thought bubble is showing, clear it when a real event happens
    if (thoughtVisible) {
        thoughtVisible = false;
        clearThoughtBubble();
    }
}

void nibblesSpeechUpdate(unsigned long currentTime) {
    // Auto-clear thought bubble after display time
    if (thoughtVisible && (currentTime - thoughtShownAt >= THOUGHT_DISPLAY_MS)) {
        thoughtVisible = false;
        clearThoughtBubble();
        return;
    }

    // Don't show new speech if an animation task is running
    if (isGlassesTaskRunning || isAngryTaskRunning || isSadTaskRunning) {
        return;
    }

    // Don't show if a thought is already visible
    if (thoughtVisible) return;

    // Check cooldown
    if (lastSpeechTime > 0 && (currentTime - lastSpeechTime < SPEECH_COOLDOWN_MS)) {
        return;
    }

    // Check if idle long enough
    if (currentTime - lastEventTime >= IDLE_TIMEOUT_MS) {
        const char* msg = pickRandom(idleMessages, idleMessageCount);
        showMumble(msg);
        // Reset idle timer so next idle speech waits again
        lastEventTime = currentTime;
    }
}

void nibblesSpeechShow(SpeechContext context) {
    unsigned long now = millis();

    // Check cooldown
    if (lastSpeechTime > 0 && (now - lastSpeechTime < SPEECH_COOLDOWN_MS)) {
        return;
    }

    const char* msg = nullptr;

    switch (context) {
        case SpeechContext::SCAN_START:
            msg = pickRandom(scanStartMessages, scanStartMessageCount);
            break;
        case SpeechContext::WARDRIVING:
            msg = pickRandom(wardrivingMessages, wardrivingMessageCount);
            break;
        case SpeechContext::SUSPICIOUS:
            msg = pickRandom(suspiciousMessages, suspiciousMessageCount);
            break;
        case SpeechContext::LEVEL_UP:
            msg = pickRandom(levelUpMessages, levelUpMessageCount);
            break;
        case SpeechContext::IDLE:
            msg = pickRandom(idleMessages, idleMessageCount);
            break;
    }

    if (msg) {
        showMumble(msg);
        lastEventTime = now;
    }
}
