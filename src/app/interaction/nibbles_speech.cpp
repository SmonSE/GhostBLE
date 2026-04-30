#include "nibbles_speech.h"

#include "infrastructure/platform/hardware.h"
#include "ui/overlay/draw_overlay.h"
#include "ui/expression/show_expression.h"
#include "app/context/globals.h"
#include "config/ui_config.h"
#include "app/context/scan_context.h"
#include "app/context/ui_context.h"

#include "assets/nibblesAngry.h"
#include "assets/nibblesFront.h"
#include "assets/nibblesHappy.h"
#include "assets/nibblesHappyLeft.h"
#include "assets/nibblesSleep.h"
#include "assets/nibblesFunny.h"
#include "assets/nibblesBored.h"
#include "assets/nibblesBoredLeft.h"


// --- Message pools ---
// max length of bubble is 16 chars, so keep it short and sweet!
static const char* idleMessages[] = {
    "Hmm...",
    "Anyone here?",
    "Nothing to do...",
    "Press H for help!", 
    "Where is everyone",
    "So quiet...",
    "Zzz...",
    "Hello?",
    "*yawn*",
    "So Bored...",
    "Beep boop...",
    "Just chillin'",
    "Wake me up!",
    "Bored.exe running"
};
static const int idleMessageCount = sizeof(idleMessages) / sizeof(idleMessages[0]);

static const char* evilMessages[] = {
    "Evil mode active",
    "Chaos engaged",
    "Lets break things",
    "I regret nothing",
    "No rules detected",
    "Maximum chaos",
    "Because we can",
    "All systems bad",
    "What could break?",
    "Oops",
    "Bad idea running",
    "More chaos pls",
    "No control left",
    "Trust me, ok?",
    "Now it gets fun",
    "Why not?"
};
static const int evilMessageCount = sizeof(evilMessages) / sizeof(evilMessages[0]);

static const char* scanStartMessages[] = {
    "Lets go!",
    "I smell BLE!",
    "Ready?",
    "Lets begin!",
    "Eyes open!",
    "Here we go!"
};
static const int scanStartMessageCount = sizeof(scanStartMessages) / sizeof(scanStartMessages[0]);

static const char* scanStopMessages[] = {
    "Scan stopped!",
    "No more scanning",
    "Time to rest!",
    "Scan complete!",
    "Eyes closed!",
    "One more scan?"
};
static const int scanStopMessageCount = sizeof(scanStopMessages) / sizeof(scanStopMessages[0]);

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
    UIContext::isSpeechBubbleActive = false;

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

    if(UIContext::isEvilModeActive) 
    {
        int r = random(2);
        if (r == 0) {
            drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                        nibblesBoredLeft, NIBBLESBOREDLEFT_WIDTH, NIBBLESBOREDLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
        } else {
            drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                        nibblesBored, NIBBLESBORED_WIDTH, NIBBLESBORED_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
        }    
    } else {
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
    }

    // Stats neu zeichnen
    showFindingCounter(ScanContext::targetConnects, ScanContext::susDevice, ScanContext::allSpottedDevice);
}

static void showMumble(const char* message) {
    if(!ScanContext::scanIsRunning){
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
    if (UIContext::isGlassesTaskRunning || UIContext::isAngryTaskRunning || UIContext::isSadTaskRunning) {
        return;
    }

    // Don't show if a thought is already visible
    if (thoughtVisible) return;

    // Check cooldown
    if (lastSpeechTime > 0 && (currentTime - lastSpeechTime < SPEECH_COOLDOWN_MS)) {
        return;
    }

    // Check if idle long enough
    if (currentTime - lastEventTime >= IDLE_TIMEOUT_MS && !ScanContext::bleScanEnabled) {
        if(UIContext::isEvilModeActive) {
            // idleMessage
            const char* msg = pickRandom(evilMessages, evilMessageCount);  
            showMumble(msg);
        } else {
            // idleMessage
            const char* msg = pickRandom(idleMessages, idleMessageCount);  
            showMumble(msg);
        }
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
        case SpeechContext::SCAN_STOP:
            msg = pickRandom(scanStopMessages, scanStopMessageCount);
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
    UIContext::isSpeechBubbleActive = true;

    if (lastSpeechTime > 0 && (now - lastSpeechTime < SPEECH_COOLDOWN_MS)) {
        return;
    }

    if (message) {
        showMumble(message);
        lastEventTime = now;
    }
}
