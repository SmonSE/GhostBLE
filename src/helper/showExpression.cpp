#include <M5Cardputer.h>
#include <M5Unified.h>
#include "showExpression.h"
#include "../globals/globals.h"
#include "../config/config.h"
#include "../helper/drawOverlay.h"

#include "../images/nibblesFront.h"
#include "../images/nibblesGlasses.h"
#include "../images/nibblesAngry.h"
#include "../images/nibblesSad.h"
#include "../images/nibblesHeartLeft.h"
#include "../images/nibblesHeartRight.h"

void showGlassesExpressionTask(void* parameter) {
    isGlassesTaskRunning = true;
    drawOverlay(nibblesGlasses, NIBBLESGLASSES_WIDTH, NIBBLESGLASSES_HEIGHT, 77, 52);
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  
  
    isGlassesTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}
  
  
void showAngryExpressionTask(void* parameter) {
    isAngryTaskRunning = true;
    drawOverlay(nibblesAngry, NIBBLESANGRY_WIDTH, NIBBLESANGRY_HEIGHT, 77, 42);
    vTaskDelay(pdMS_TO_TICKS(3000));  // 3 Sekunden
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  
  
    isAngryTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}
  
  
void showSadExpressionTask(void* parameter) {
    isSadTaskRunning = true;
    drawOverlay(nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 77, 42);
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  
  
    isSadTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}
  
void showBatteryState() {
  // Read battery voltage
  float voltage = M5.Power.getBatteryVoltage(); // e.g., 4.1V

  // Convert voltage to approximate percentage
  int batteryPercent = map(voltage * 100, 330, 420, 0, 100); // 3.3V = 0%, 4.2V = 100%
  batteryPercent = constrain(batteryPercent, 0, 100);

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(210, 10);
  M5.Lcd.printf("%d%%", batteryPercent);
}
  
void showFindingCounter(int sniffed, int susDevice, int spotted) {

  showBatteryState();

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(5, 124);
  M5.Lcd.print("Spotted:");
  M5.Lcd.println(spotted);
  
  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(100, 124);
  M5.Lcd.print("Sniffed:");
  M5.Lcd.println(sniffed);
  
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(190, 124);
  M5.Lcd.print("Sus:");
  M5.Lcd.println(susDevice);
}