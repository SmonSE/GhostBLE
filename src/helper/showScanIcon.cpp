#include "showScanIcon.h"
#include "showExpression.h"
#include "config/scanConfig.h"
#include "config/hardware.h"
#include "globals/globals.h"
#include "config/config.h"
#include "helper/drawOverlay.h"
#include "logger/logger.h"


// Position vom Icon
#define ICON_X 190
#define ICON_Y 105
#define ICON_WIDTH 30
#define ICON_HEIGHT 20
#define BG_COLOR 0x00C4

void drawBars(int x, int y, int level) {
  int barWidth = 6;
  int spacing = 4;

  uint16_t activeColor;

  switch (level) {
    case 4:
      activeColor = TFT_RED;
      break;
    case 3:
      activeColor = TFT_YELLOW;
      break;
    case 2:
    default:
      activeColor = TFT_GREEN;
      break;
  }

  for (int i = 0; i < 4; i++) {
    int barHeight = (i + 1) * 6;

    uint16_t color = (i < level) ? activeColor : TFT_DARKGREY;

    M5.Display.fillRect(
      x + i * (barWidth + spacing),
      y - barHeight,
      barWidth,
      barHeight,
      color
    );
  }
}

// Icon löschen (optional)
void clearScanIcon() {
  M5.Lcd.fillRect(ICON_X, ICON_Y, ICON_WIDTH, ICON_HEIGHT, BG_COLOR);
}

// Hauptfunktion
void showScanIcon() {

  // Erst löschen
  clearScanIcon();

  switch (currentMode) {
    case BALANCED:
      drawBars(ICON_X, ICON_Y, 2);
      break;

    case AGGRESSIVE:
      drawBars(ICON_X, ICON_Y, 4);
      break;

    case FOCUSED:
      drawBars(ICON_X, ICON_Y, 3);
      break;
  }
}