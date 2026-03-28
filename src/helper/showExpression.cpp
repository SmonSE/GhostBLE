#include "../config/hardware.h"
#include "showExpression.h"
#include "../globals/globals.h"
#include "../config/config.h"
#include "../helper/drawOverlay.h"

#include "../images/nibblesFront.h"
#include "../images/nibblesGlasses.h"
#include "../images/nibblesAngry.h"
#include "../images/nibblesSad.h"
#include "../images/nibblesHappy.h"
#include "../images/nibblesHappyLeft.h"
#include "../images/nibblesHeartLeft.h"
#include "../images/nibblesHeartRight.h"
#include "../images/nibblesThugLife.h"
#include "../images/nibblesSleep.h"
#include "../gps/GPSManager.h"

static float smoothedVoltage = 0;
static int displayedPercent = 100;
static bool lastChargingState = false;
static unsigned long usbDisconnectTime = 0;

static const struct { int mv; int percent; } batteryLevels[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3900, 70},
    {3800, 60}, {3700, 50}, {3600, 30}, {3500, 20}, {3400, 10}
};

static int voltageToPercent(int mv) {
    for (const auto& level : batteryLevels) {
        if (mv >= level.mv) return level.percent;
    }
    return 5;
}

// --- Icon drawing functions ---

static void drawWifiIcon(int x, int y, bool active) {
  uint16_t color = active ? GREEN : 0x4208;
  M5.Lcd.fillRect(x,     y + 6, 2, 3, color);
  M5.Lcd.fillRect(x + 3, y + 3, 2, 6, color);
  M5.Lcd.fillRect(x + 6, y,     2, 9, color);
}

static void drawScanIcon(int x, int y, bool active) {
  uint16_t color = active ? GREEN : 0x4208;
  M5.Lcd.fillRect(x + 2, y,     1, 1, color);
  M5.Lcd.fillRect(x + 1, y + 1, 3, 1, color);
  M5.Lcd.fillRect(x,     y + 2, 5, 1, color);
  M5.Lcd.fillRect(x + 1, y + 3, 3, 1, color);
  M5.Lcd.fillRect(x + 2, y + 4, 1, 1, color);
}

static void drawGPSIcon(int x, int y, bool hasFix) {
  uint16_t color = hasFix ? GREEN : RED;
  M5.Lcd.drawCircle(x + 4, y + 4, 4, color);
  M5.Lcd.drawLine(x + 4, y, x + 4, y + 8, color);
  M5.Lcd.drawLine(x, y + 4, x + 8, y + 4, color);
  if (hasFix) M5.Lcd.fillCircle(x + 4, y + 4, 1, color);
}

static void drawBatteryIcon(int x, int y, int percent, bool charging) {
  M5.Lcd.drawRect(x, y, 16, 8, WHITE);
  M5.Lcd.fillRect(x + 16, y + 2, 2, 4, WHITE);
  uint16_t fillColor = charging ? YELLOW : (percent < 20 ? RED : GREEN);
  int fillW = 14 * percent / 100;
  if (fillW > 0) M5.Lcd.fillRect(x + 1, y + 1, fillW, 6, fillColor);
  if (fillW < 14) M5.Lcd.fillRect(x + 1 + fillW, y + 1, 14 - fillW, 6, BLACK);
}

static void updateBatteryState() {
  int rawVoltage = M5.Power.getBatteryVoltage();

  bool chargingNow = M5.Power.isCharging();
  bool usbConnected = (rawVoltage > 4200);

  if (!chargingNow && rawVoltage > 4200) {
    chargingNow = true;
  }

  // Smooth Voltage
  if (smoothedVoltage == 0) {
    smoothedVoltage = rawVoltage;
  }

  smoothedVoltage = smoothedVoltage * 0.92f + rawVoltage * 0.08f;

  // USB Disconnect Delay Fix
  if (isChargingState && !chargingNow) {
    usbDisconnectTime = millis();
  }

  isChargingState = chargingNow || usbConnected;

  if (!chargingNow && (millis() - usbDisconnectTime < 2000)) {
    // keep value
  } else {
    int newPercent = voltageToPercent((int)smoothedVoltage);

    if (newPercent > displayedPercent)
      displayedPercent++;
    else if (newPercent < displayedPercent)
      displayedPercent--;
  }
}


void drawHeart(int x, int y, uint16_t color) {

    int s = 2; // pixel size (scaling)

    M5.Display.fillRect(x+2*s, y+0*s, s, s, color);
    M5.Display.fillRect(x+3*s, y+0*s, s, s, color);
    M5.Display.fillRect(x+6*s, y+0*s, s, s, color);
    M5.Display.fillRect(x+7*s, y+0*s, s, s, color);

    M5.Display.fillRect(x+1*s, y+1*s, s, s, color);
    M5.Display.fillRect(x+4*s, y+1*s, s, s, color);
    M5.Display.fillRect(x+5*s, y+1*s, s, s, color);
    M5.Display.fillRect(x+8*s, y+1*s, s, s, color);

    M5.Display.fillRect(x+0*s, y+2*s, s, s, color);
    M5.Display.fillRect(x+9*s, y+2*s, s, s, color);

    M5.Display.fillRect(x+1*s, y+3*s, s, s, color);
    M5.Display.fillRect(x+8*s, y+3*s, s, s, color);

    M5.Display.fillRect(x+2*s, y+4*s, s, s, color);
    M5.Display.fillRect(x+7*s, y+4*s, s, s, color);

    M5.Display.fillRect(x+3*s, y+5*s, s, s, color);
    M5.Display.fillRect(x+6*s, y+5*s, s, s, color);

    M5.Display.fillRect(x+4*s, y+6*s, s, s, color);
    M5.Display.fillRect(x+5*s, y+6*s, s, s, color);
}

void clearHearts() {
    M5.Display.fillRect(25, 24, 40, 30, 0x00C4);
}

void clearSpeechBubble() {

    int srcX = BUBBLE_X - NIBBLES_FRONT_X;
    int srcY = BUBBLE_RECT_Y - NIBBLES_FRONT_Y;

    int restoreH = BUBBLE_RECT_H + BUBBLE_TRI_H + 2;

    for (int row = 0; row < restoreH; row++) {
        M5.Lcd.pushImage(
            BUBBLE_X,
            BUBBLE_RECT_Y + row,
            BUBBLE_MAX_W,
            1,
            &nibblesFront[(srcY + row) * NIBBLESFRONT_WIDTH + srcX]
        );
    }

    drawComposite(
        nibblesFront,
        NIBBLESFRONT_WIDTH,
        NIBBLES_FRONT_X,
        NIBBLES_FRONT_Y,
        nibblesHappy,
        NIBBLESHAPPY_WIDTH,
        NIBBLESHAPPY_HEIGHT,
        NIBBLES_HAPPY_X,
        NIBBLES_HAPPY_Y
    );

    showFindingCounter(targetConnects, susDevice, allSpottedDevice);
}


void showGlassesExpressionTask(void* parameter) {
    isGlassesTaskRunning = true;
    drawOverlay(nibblesGlasses, NIBBLESGLASSES_WIDTH, NIBBLESGLASSES_HEIGHT, 76, 52);

    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    // HappyLeft and Happy should be shown randomly to add some variety
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    }
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);

    // Fallback chain: localName → deviceName → appearanceName
    String bubbleText = localName;
    if (bubbleText.length() == 0) bubbleText = deviceName;
    if (bubbleText.length() == 0) bubbleText = appearanceName;

    if (bubbleText.length() > 0 && !isSpeechBubbleActive) {
      clearSpeechBubble();
      if(bubbleText.length() > 14) {
        bubbleText = bubbleText.substring(0, 11) + "...";
      }

      drawBubble(bubbleText.c_str(), BUBBLE_X, BUBBLE_RECT_Y, WHITE, BUBBLE_BORDER_COLOR, BLACK);

      vTaskDelay(pdMS_TO_TICKS(3000));  // 3 Sekunden

    } else if (appearanceName.length() > 0 && !isSpeechBubbleActive && localName.length() == 0) {
      clearSpeechBubble();
      if(appearanceName.length() > 14) {
        appearanceName = appearanceName.substring(0, 11) + "...";
      } 

      drawBubble(appearanceName.c_str(), BUBBLE_X, BUBBLE_RECT_Y, WHITE, BUBBLE_BORDER_COLOR, BLACK);

      vTaskDelay(pdMS_TO_TICKS(3000));  // 3 Sekunden
    }

    clearSpeechBubble();

    isGlassesTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}


void showAngryExpressionTask(void* parameter) {
    isAngryTaskRunning = true;
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                  nibblesAngry, NIBBLESANGRY_WIDTH, NIBBLESANGRY_HEIGHT, 83, 60);

    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    // HappyLeft and Happy should be shown randomly to add some variety
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    }
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);

    isAngryTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden

    clearSpeechBubble();
}


void showSadExpressionTask(void* parameter) {
    isSadTaskRunning = true;
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                  nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);

    // Fallback chain: localName → deviceName → appearanceName
    String sadBubbleText = localName;
    if (sadBubbleText.length() == 0) sadBubbleText = deviceName;
    if (sadBubbleText.length() == 0) sadBubbleText = appearanceName;

    if (sadBubbleText.length() > 0 && !isSpeechBubbleActive) {
      clearSpeechBubble();
      if(sadBubbleText.length() > 14) {
        sadBubbleText = sadBubbleText.substring(0, 11) + "...";
      }
      drawBubble(sadBubbleText.c_str(), BUBBLE_X, BUBBLE_RECT_Y, WHITE, BUBBLE_BORDER_COLOR, BLACK);
    }

    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    // HappyLeft and Happy should be shown randomly to add some variety
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    }
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);

    isSadTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden

    clearSpeechBubble();
}

void showHappyExpressionTask(void* parameter) {
    isHappyTaskRunning = true;
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    // HappyLeft and Happy should be shown randomly to add some variety
    if (random(2) == 0) {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    } else {
      drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                    nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
    }
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);

    vTaskDelay(pdMS_TO_TICKS(4000));  // 4 Sekunden

    isHappyTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden

    clearSpeechBubble();
}


void showThugLifeExpressionTask(void* parameter) {
  isThugLifeTaskRunning = true;
  drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);

  vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

  // HappyLeft and Happy should be shown randomly to add some variety
  if (random(2) == 0) {
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                  nibblesHappyLeft, NIBBLESHAPPYLEFT_WIDTH, NIBBLESHAPPYLEFT_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
  } else {
    drawComposite(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLES_FRONT_X, NIBBLES_FRONT_Y,
                  nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, NIBBLES_HAPPY_X, NIBBLES_HAPPY_Y);
  }
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);

  isThugLifeTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}

static void drawStatusIcons(int x, int y) {
  if (wardrivingEnabled) {
    extern GPSManager gpsManager;
    bool hasFix = gpsManager.isValid();
    drawGPSIcon(x, y, hasFix);
    uint16_t gpsColor = hasFix ? GREEN : RED;
    M5.Lcd.setTextColor(gpsColor);
    M5.Lcd.setCursor(x + 13, y + 2);
    M5.Lcd.printf("SAT:%u %s", gpsManager.getSatellites(), hasFix ? "FIX" : "NO FIX");
  } else {
    drawWifiIcon(x, y, isWebLogActive);
    drawScanIcon(x + 15, y + 2, bleScanEnabledWeb);
  }
}

static void drawStats(int sniffed, int susDevice, int spotted, int x, int y) {

  M5.Lcd.setTextColor(WHITE, 0x00C4);

  M5.Lcd.setCursor(x, y);
  M5.Lcd.printf("Spt %-4d", spotted);

  M5.Lcd.setCursor(x, y + STATS_LINE_HEIGHT);
  M5.Lcd.printf("Snf %-4d", sniffed);

  M5.Lcd.setCursor(x, y + STATS_LINE_HEIGHT * 2);
  M5.Lcd.printf("Bcn %-4d", beaconsFound.load());

  M5.Lcd.setTextColor(RED, 0x00C4);
  M5.Lcd.setCursor(x, y + STATS_LINE_HEIGHT * 3);
  M5.Lcd.printf("Sus %-4d", susDevice);
}

static void drawXPBar(int x, int y) {
  M5.Lcd.setTextColor(GREEN, 0x00C4);
  M5.Lcd.setCursor(x, y);
  M5.Lcd.printf("LV%u", xpManager.getLevel());

  // Progress bar
  M5.Lcd.drawRect(XP_BAR_X, y, XP_BAR_W, XP_BAR_H, GREEN);
  int fillW = (XP_BAR_W - 2) * xpManager.getProgressPercent() / 100;
  if (fillW > 0) {
    M5.Lcd.fillRect(XP_BAR_X + 1, y + 1, fillW, XP_BAR_H - 2, GREEN);
  }
  if (fillW < XP_BAR_W - 2) {
    M5.Lcd.fillRect(XP_BAR_X + 1 + fillW, y + 1, XP_BAR_W - 2 - fillW, XP_BAR_H - 2, BLACK);
  }

  // Clear title area
  M5.Lcd.fillRect(TITLE_TEXT_X, y, 140, 16, 0x00C4);

  // Nibbles title
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(TITLE_TEXT_X, y);
  M5.Lcd.print(xpManager.getTitle());
}

void showFindingCounter(int sniffed, int susDevice, int spotted) {

  updateBatteryState();
  bool charging = M5.Power.isCharging();
  M5.Lcd.setTextSize(1);

  // ---- TOP BAR ----
  drawStatusIcons(STATUS_ICON_X, STATUS_BAR_Y);
  drawBatteryIcon(215, STATUS_BAR_Y, displayedPercent, isChargingState);

  // ---- STATS — LEFT SIDE ----
  drawStats(sniffed, susDevice, spotted, STATS_X, STATS_Y_START);

  // ---- BOTTOM BAR — Level/XP ----
  drawXPBar(LEVEL_TEXT_X, BOTTOM_BAR_Y);
}
