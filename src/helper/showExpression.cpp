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
#include "../images/nibblesHappy.h"
#include "../images/nibblesHeartLeft.h"
#include "../images/nibblesHeartRight.h"
#include "../images/nibblesThugLife.h"
#include "src/images/nibblesBubble.h"

static float smoothedVoltage = 0;
static int displayedPercent = 100;
static bool lastChargingState = false;
static unsigned long usbDisconnectTime = 0;


void showGlassesExpressionTask(void* parameter) {
    isGlassesTaskRunning = true;
    drawOverlay(nibblesGlasses, NIBBLESGLASSES_WIDTH, NIBBLESGLASSES_HEIGHT, 76, 52);
    //showFindingCounter(targetConnects, susDevice, leakedCounter);  

    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);

    if (localName.length() > 0) {
      if(localName.length() > 14) {
        localName = localName.substring(0, 15) + "...";
      }
      drawOverlay(speechBubble, SPEECHBUBBLE_WIDTH, SPEECHBUBBLE_HEIGHT, 130, 15);
      delay(200);
      M5.Lcd.setTextColor(BLACK); 
      //M5.Lcd.setFont(&fonts::Font2); 
      M5.Lcd.setTextSize(1); 
      M5.Lcd.setCursor(140, 27);
      M5.Lcd.println(localName.c_str());
      vTaskDelay(pdMS_TO_TICKS(3000));  // 3 Sekunden
    }
  
    isGlassesTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}

  
void showAngryExpressionTask(void* parameter) {
    isAngryTaskRunning = true;
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesAngry, NIBBLESANGRY_WIDTH, NIBBLESANGRY_HEIGHT, 83, 60);
    //showFindingCounter(targetConnects, susDevice, leakedCounter);  

    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  
  
    isAngryTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}
  
  
void showSadExpressionTask(void* parameter) {
    isSadTaskRunning = true;
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  

    if (localName.length() > 0) {
      if(localName.length() > 14) {
        localName = localName.substring(0, 15) + "...";
      }
      drawOverlay(speechBubble, SPEECHBUBBLE_WIDTH, SPEECHBUBBLE_HEIGHT, 130, 15);
      delay(200);
      M5.Lcd.setTextColor(BLACK); 
      //M5.Lcd.setFont(&fonts::Font2); 
      M5.Lcd.setTextSize(1); 
      M5.Lcd.setCursor(140, 27);
      M5.Lcd.println(localName.c_str());
    }

    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  

    isSadTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}

void showHappyExpressionTask(void* parameter) {
    isHappyTaskRunning = true;
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice);  

    vTaskDelay(pdMS_TO_TICKS(4000));  // 4 Sekunden

    isHappyTaskRunning = false;
    vTaskDelete(NULL);  // Task selbst beenden
}
  
  
void showThugLifeExpressionTask(void* parameter) {
  isThugLifeTaskRunning = true;
  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
  //showFindingCounter(targetConnects, susDevice, leakedCounter);  

  vTaskDelay(pdMS_TO_TICKS(2000));  // 2 Sekunden

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  showFindingCounter(targetConnects, susDevice, allSpottedDevice);  

  isThugLifeTaskRunning = false;
  vTaskDelete(NULL);  // Task selbst beenden
}

int voltageToPercent(int mv) {

  if (mv >= 4200) return 100;
  if (mv >= 4100) return 90;
  if (mv >= 4000) return 80;
  if (mv >= 3950) return 70;
  if (mv >= 3900) return 60;
  if (mv >= 3850) return 50;
  if (mv >= 3800) return 40;
  if (mv >= 3750) return 30;
  if (mv >= 3700) return 20;
  if (mv >= 3600) return 10;
  if (mv >= 3500) return 5;

  return 0;
}

void showBatteryState() {

  int rawVoltage = M5.Power.getBatteryVoltage(); // mV
  bool charging = M5.Power.isCharging();

  // ---- 1. Spannung glätten ----
  if (smoothedVoltage == 0) {
    smoothedVoltage = rawVoltage; // Initialisieren
  }

  // Exponentielle Glättung (ruhig aber noch responsiv)
  smoothedVoltage = smoothedVoltage * 0.92f + rawVoltage * 0.08f;

  // ---- 2. USB Disconnect erkennen ----
  if (lastChargingState && !charging) {
    usbDisconnectTime = millis();   // Zeitpunkt merken
  }

  lastChargingState = charging;

  // 2 Sekunden nach USB-Abziehen Anzeige einfrieren
  if (!charging && (millis() - usbDisconnectTime < 2000)) {
    // nichts tun → alten Prozentwert behalten
  } else {
    int newPercent = voltageToPercent((int)smoothedVoltage);

    // ---- 3. Langsame Prozent-Annäherung (Anti-Sprung) ----
    if (newPercent > displayedPercent)
      displayedPercent++;
    else if (newPercent < displayedPercent)
      displayedPercent--;
  }

  // ---- Anzeige ----
  M5.Lcd.setCursor(210, 5);
  M5.Lcd.setTextSize(1);

  if (charging) {
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.printf("%d%%", displayedPercent);
  } else {
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf("%d%%", displayedPercent);
  }
}
  
void showFindingCounter(int sniffed, int susDevice, int spotted) {

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(5, 5);
  M5.Lcd.print("Wifi:");
  M5.Lcd.println(isWebLogActive ? "ON" : "OFF");

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(100, 5);
  M5.Lcd.print("Scan:");
  M5.Lcd.println(bleScanEnabledWeb ? "ON" : "OFF");

  showBatteryState();

  // Wardriving status line
  if (wardrivingEnabled) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(5, 94);
    M5.Lcd.print("WD:ON");
  }

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(5, 104);
  M5.Lcd.print("Beacons:");
  M5.Lcd.println(beaconsFound);

  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(5, 124);
  M5.Lcd.print("Sniffed:");
  M5.Lcd.println(sniffed);
  
  M5.Lcd.setTextColor(WHITE); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(100, 124);
  M5.Lcd.print("Spotted:");
  M5.Lcd.println(spotted);
  
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(190, 124);
  M5.Lcd.print("Sus:");
  M5.Lcd.println(susDevice);
}