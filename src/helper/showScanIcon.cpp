#include "showScanIcon.h"
#include "showExpression.h"
#include "../config/scanConfig.h"
#include "../config/hardware.h"
#include "../globals/globals.h"
#include "../config/config.h"
#include "../helper/drawOverlay.h"
#include "../images/scanIcon.h"
#include "../logger/logger.h"


// Position vom Icon
#define ICON_X 190
#define ICON_Y 80

// Icon löschen (optional)
void clearScanIcon() {
  M5.Lcd.fillRect(ICON_X, ICON_Y, ICON_WIDTH, ICON_HEIGHT, 0x00C4);
}

// Hauptfunktion
void showScanIcon() {
  const uint16_t* icon = nullptr;

  switch (currentMode) {
    case BALANCED:
      icon = iconBalanced;
      break;

    case AGGRESSIVE:
      icon = iconAggressive;
      break;

    case FOCUSED:
      icon = iconFocus;
      break;
  }

  // Optional löschen (kannst du auch auskommentieren wenn kein Flicker)
  clearScanIcon();

  if (icon != nullptr) {
    M5.Lcd.pushImage(ICON_X, ICON_Y, ICON_WIDTH, ICON_HEIGHT, (uint16_t*)icon);
  }
}