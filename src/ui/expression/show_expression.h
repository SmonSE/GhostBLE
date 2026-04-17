#pragma once

#include <Arduino.h>

enum ScanState {
  SCAN_OFF,
  SCAN_RUNNING,
  SCAN_STOPPING
};

void drawPointer(int pointer);
void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showThugLifeExpressionTask(void* parameter);
void showHappyExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
void drawHeart(int x, int y, uint16_t color);
void clearHearts();
void clearSpeechBubble();
void showHelpOverlay();
void dismissHelpOverlay();
void drawStatusIcons(int x, int y);
void drawBatteryIcon(int x, int y, int percent, bool isCharging);
void updateBatteryState();
void drawGPSIcon(int x, int y, bool hasFix);
void drawScanIcon(int x, int y, ScanState state, int radius);
void drawWifiIcon(int x, int y, bool active);
void drawStats(int sniffed, int susDevice, int spotted, int x, int y);
void drawXPBar(int x, int y);
