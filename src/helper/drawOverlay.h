#pragma once

#include <M5Cardputer.h>

// Function to draw an overlay image onto the screen
// Parameters:
// - img: pointer to the image data (in 16-bit color format)
// - w: width of the image
// - h: height of the image
// - x0: x-coordinate to start drawing
// - y0: y-coordinate to start drawing
void drawOverlay(const uint16_t* img, int w, int h, int x0, int y0);

// Draw a speech bubble with a triangle pointer at the given position.
void drawBubble(const char* message, int x0, int y0,
                uint16_t fillColor, uint16_t borderColor, uint16_t textColor);
