#include "drawOverlay.h"
#include <M5Unified.h>

void drawOverlay(const uint16_t* img, int w, int h, int x0, int y0) {
    M5.Lcd.pushImage(x0, y0, w, h, img, (uint16_t)0xFFFF);
  }

void drawBubble(const char* message, int x0, int y0,
                uint16_t fillColor, uint16_t borderColor, uint16_t textColor) {
    int textLen = strlen(message);
    // Bubble width adapts to text (min 60, max 108 to fit screen)
    int bubbleW = min(108, max(60, textLen * 6 + 16));
    int bubbleH = 22;

    // Draw rounded rect bubble
    M5.Lcd.fillRoundRect(x0, y0, bubbleW, bubbleH, 4, fillColor);
    M5.Lcd.drawRoundRect(x0, y0, bubbleW, bubbleH, 4, borderColor);

    // Small triangle pointer toward NibBLEs (bottom-left)
    int triX = x0 + 8;
    int triY = y0 + bubbleH;
    M5.Lcd.fillTriangle(triX, triY - 1, triX + 6, triY - 1, triX, triY + 4, fillColor);
    M5.Lcd.drawLine(triX, triY - 1, triX, triY + 4, borderColor);
    M5.Lcd.drawLine(triX, triY + 4, triX + 6, triY - 1, borderColor);

    // Draw text
    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x0 + 6, y0 + 7);
    M5.Lcd.print(message);
}