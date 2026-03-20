#include "screenshot.h"
#include <SD.h>

uint16_t* Screenshot::buffer = nullptr;
bool Screenshot::pending = false;

int Screenshot::width = 0;
int Screenshot::height = 0;

unsigned long Screenshot::messageUntil = 0;

void Screenshot::init() {
    width  = M5Cardputer.Display.width();
    height = M5Cardputer.Display.height();

    if (!SD.begin()) {
        Serial.println("[Screenshot] SD init failed!");
    } else {
        Serial.println("[Screenshot] SD ready");
    }
}

void Screenshot::capture() {
    if (pending) return; // no spam

    if (!buffer) {
        buffer = (uint16_t*)malloc(width * height * sizeof(uint16_t));
        if (!buffer) {
            Serial.println("[Screenshot] No RAM!");
            return;
        }
    }

    // Direct read from display framebuffer
    M5Cardputer.Display.readRect(0, 0, width, height, buffer);

    pending = true;

    Serial.println("[Screenshot] Captured");
}

void Screenshot::handle() {
    // Save pending screenshot to SD
    if (pending) {
        char filename[64];
        sprintf(filename, "/GhostBLE/shot_%lu_%u.raw", millis(), esp_random() % 10000);

        File file = SD.open(filename, FILE_WRITE);
        if (!file) {
            Serial.println("[Screenshot] File open failed");
            pending = false;
            return;
        }

        Serial.printf("[Screenshot] Saving %s\n", filename);

        file.write((uint8_t*)buffer, width * height * 2);
        file.close();

        pending = false;

        Serial.println("[Screenshot] Done");

        // Show UI news for 2 seconds
        messageUntil = millis() + 2000;

        M5Cardputer.Display.setCursor(90, 0);
        M5Cardputer.Display.println("PrtScn saved");
    }

    // Delete message after timeout
    if (messageUntil > 0 && millis() > messageUntil) {
        M5Cardputer.Display.fillRect(88, 0, 80, 12, 0x00C4);
        messageUntil = 0;
    }
}