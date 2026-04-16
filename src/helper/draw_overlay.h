#pragma once

#include "config/hardware.h"

// Function to draw an overlay image onto the screen
// Parameters:
// - img: pointer to the image data (in 16-bit color format)
// - w: width of the image
// - h: height of the image
// - x0: x-coordinate to start drawing
// - y0: y-coordinate to start drawing
void drawOverlay(const uint16_t* img, int w, int h, int x0, int y0);

// Sprite-composited expression drawing.  Restores the base image region
// covering all possible expression positions, composites the overlay on
// top, and pushes the result in a single SPI transfer.  ~10x fewer pixels
// than redrawing the full base image.
void drawComposite(const uint16_t* base, int baseW, int baseX, int baseY,
                   const uint16_t* overlay, int overlayW, int overlayH,
                   int overlayX, int overlayY);

// Draw a speech bubble with a triangle pointer at the given position.
void drawBubble(const char* message, int x0, int y0,
                uint16_t fillColor, uint16_t borderColor, uint16_t textColor);
