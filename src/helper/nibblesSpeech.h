#ifndef NIBBLES_SPEECH_H
#define NIBBLES_SPEECH_H

#include <Arduino.h>

// Speech context categories
enum class SpeechContext {
    IDLE,
    SCAN_START,
    WARDRIVING,
    SUSPICIOUS,
    LEVEL_UP
};

// Initialize the speech system (call in setup)
void nibblesSpeechBegin();

// Call from loop() to check if NibBLEs should mumble
void nibblesSpeechUpdate(unsigned long currentTime);

// Notify the speech system that something happened (resets idle timer)
void nibblesSpeechNotifyEvent();

// Show a context-aware speech bubble immediately (with cooldown check)
void nibblesSpeechShow(SpeechContext context);

// Draw a thought bubble (green tint) with the given message
void drawThoughtBubble(const char* message, int x0, int y0);

#endif // NIBBLES_SPEECH_H
