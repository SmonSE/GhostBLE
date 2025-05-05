#include "drawOverlay.h"

void drawOverlay(const uint16_t* img, int w, int h, int x0, int y0) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        uint16_t color = img[y * w + x];
        if (color != 0xFFFF) {  // 0xFFFF = transparent
          M5.Lcd.drawPixel(x0 + x, y0 + y, color);
        }
      }
    }
  }
  