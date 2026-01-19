#ifndef SHOW_EXPRESSION_H
#define SHOW_EXPRESSION_H

#include <Arduino.h>

void showGlassesExpressionTask(void* parameter);
void showAngryExpressionTask(void* parameter);
void showSadExpressionTask(void* parameter);
void showThugLifeExpressionTask(void* parameter);
void showHappyExpressionTask(void* parameter);
void showBatteryState();
void showFindingCounter(int sniffed, int susDevice, int spotted);

#endif