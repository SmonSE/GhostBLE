#pragma once

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

// Show a custom message in the speech bubble (bypasses pool selection)
void nibblesSpeechShowCustom(const char* message);

// Draw a thought bubble (green tint) with the given message
void drawThoughtBubble(const char* message, int x0, int y0);
