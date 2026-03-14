#include "drawOverlay.h"
#include <M5Unified.h>

void drawOverlay(const uint16_t* img, int w, int h, int x0, int y0) {
    M5.Lcd.pushImage(x0, y0, w, h, img, (uint16_t)0xFFFF);
  }

// Union bounding box of all Nibbles expressions:
//   Happy(83,60,72,26) Angry(83,60,72,30) Sad(83,56,72,30)
//   Glasses(76,52,86,27) ThugLife(80,52,80,36) Sleep(83,60,72,26)
static const int EXPR_REGION_X = 76;
static const int EXPR_REGION_Y = 52;
static const int EXPR_REGION_W = 86;   // 162 - 76
static const int EXPR_REGION_H = 38;   // 90  - 52

void drawComposite(const uint16_t* base, int baseW, int baseX, int baseY,
                   const uint16_t* overlay, int overlayW, int overlayH,
                   int overlayX, int overlayY) {
    M5Canvas canvas(&M5.Lcd);
    if (!canvas.createSprite(EXPR_REGION_W, EXPR_REGION_H)) return;

    // Fill entire expression region from base image (row-by-row sub-region copy)
    int bOffX = EXPR_REGION_X - baseX;
    int bOffY = EXPR_REGION_Y - baseY;
    for (int row = 0; row < EXPR_REGION_H; row++) {
        canvas.pushImage(0, row, EXPR_REGION_W, 1,
            &base[(bOffY + row) * baseW + bOffX]);
    }

    // Composite overlay at its relative position within the region
    canvas.pushImage(overlayX - EXPR_REGION_X, overlayY - EXPR_REGION_Y,
                     overlayW, overlayH, overlay, (uint16_t)0xFFFF);

    // Single SPI transfer to LCD
    canvas.pushSprite(EXPR_REGION_X, EXPR_REGION_Y);
    canvas.deleteSprite();
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