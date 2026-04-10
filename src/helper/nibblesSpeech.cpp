#include "nibblesSpeech.h"
#include "../config/hardware.h"
#include "drawOverlay.h"
#include "showExpression.h"
#include "../globals/globals.h"
#include "../config/config.h"
#include "../images/nibblesFront.h"
#include "../images/nibblesHappy.h"
#include "../images/nibblesHappyLeft.h"
#include "../images/nibblesSleep.h"
#include "../images/nibblesFunny.h"

// --- Message pools ---

static const char* idleMessages[] = {
    "Hmm...",
    "Anyone here?",
    "Nothing to do...",
    "Press BtnG0",
    "Where is everyone",
    "So quiet...",
    "Zzz...",
    "Hello?",
    "*yawn*",
    "So Bored..."
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
    clearSpeechBubble();
    drawBubble(message, x0, y0, 0x2444, 0x07E0, 0x07E0);  // dark green fill, green border, green text
}

static void clearThoughtBubble() {
    // Clear thought bubble area
    isSpeechBubbleActive = false;

    // Hintergrund unter der Bubble wiederherstellen
    int srcX = BUBBLE_X - NIBBLES_FRONT_X;
    int srcY = THOUGHT_BUBBLE_Y - NIBBLES_FRONT_Y;

    for (int row = 0; row < 30; row++) {
        M5.Lcd.pushImage(
            BUBBLE_X,
            THOUGHT_BUBBLE_Y + row,
            BUBBLE_MAX_W,
            1,
            &nibblesFront[(srcY + row) * NIBBLESFRONT_WIDTH + srcX]
        );
    }

    // Gesicht wieder korrekt zusammensetzen
    int r = random(3);

    if (r == 0) {
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    } else if (r == 1) {
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    } else {
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesFunny, NIBBLESFUNNY_WIDTH, NIBBLESFUNNY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    }

    // Stats neu zeichnen
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);
}

static void showMumble(const char* message) {
    if(!scanIsRunning){
        // Clear any previous bubble and draw sleep expression
        M5.Lcd.fillRect(BUBBLE_X, THOUGHT_BUBBLE_Y, BUBBLE_MAX_W, 22, 0x00C4);
        drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesSleep, NIBBLESSLEEP_WIDTH, NIBBLESSLEEP_HEIGHT, NIBBLES_SLEEP_X, NIBBLES_SLEEP_Y);

        drawThoughtBubble(message, BUBBLE_X, THOUGHT_BUBBLE_Y);
        thoughtVisible = true;
        thoughtShownAt = millis();
        lastSpeechTime = millis();
    }
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
    if (currentTime - lastEventTime >= IDLE_TIMEOUT_MS && !bleScanEnabled) {
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

void nibblesSpeechShowCustom(const char* message) {
    unsigned long now = millis();
    isSpeechBubbleActive = true;

    if (lastSpeechTime > 0 && (now - lastSpeechTime < SPEECH_COOLDOWN_MS)) {
        return;
    }

    if (message) {
        showMumble(message);
        lastEventTime = now;
    }
}
