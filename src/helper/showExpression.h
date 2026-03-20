#pragma once

#include <Arduino.h>

void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showThugLifeExpressionTask(void* parameter);
void showHappyExpressionTask(void* parameter);
void showFindingCounter(int sniffed, int susDevice, int spotted);
void drawHeart(int x, int y, uint16_t color);
void clearHearts();
void clearSpeechBubble();
