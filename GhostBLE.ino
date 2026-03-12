#include <M5Cardputer.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include <NimBLEDevice.h>

#include "src/globals/globals.h"
#include "src/config/config.h"
#include "src/sdCard/SDLogger.h"
#include "src/scanner/ScanDevices.h"
#include "src/helper/drawOverlay.h"
#include "src/helper/showExpression.h"

#include "src/images/nibblesStartWorking.h"
#include "src/images/nibblesFront.h"
#include "src/images/nibblesGlasses.h"
#include "src/images/nibblesAngry.h"
#include "src/images/nibblesSad.h"
#include "src/images/nibblesHeartLeft.h"
#include "src/images/nibblesHeartRight.h"
#include "src/images/nibblesHappy.h"
#include "src/images/nibblesThugLife.h"
#include "src/images/nibblesBubble.h"

#include "src/logToSerialAndWeb/logger.h"

#include <WiFi.h>
#include <AsyncTCP.h>


unsigned long startTimeDevice;
const unsigned long timerDurationDevice = 60 * 60 * 1000; // 60 Minuten in Millisekunden

const char* ap_ssid = "ESP32-Log";
const char* ap_password = "12345678";

AsyncWebServer server(80);

auto keys = M5Cardputer.Keyboard.keysState();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client connected: %u\n", client->id());
  }
}

void playMysteryBoot() {
  M5.Speaker.setVolume(60);

  int notes[] = { 440, 622, 880, 740, 1046 };
  int durations[] = { 180, 120, 150, 220, 300 };

  for (int i = 0; i < 5; i++) {
    M5.Speaker.tone(notes[i], durations[i]);
    delay(durations[i] + 40);
  }

  // low bass finish
  M5.Speaker.tone(220, 400);
}

void playNotificationPro() {
  M5.Speaker.setVolume(40);

  int notes[] = { 1568, 1865, 2093 }; // G6, A#6, C7
  int dur = 80;

  for (int i = 0; i < 3; i++) {
    M5.Speaker.tone(notes[i], dur);
    delay(dur + 40);
  }
}

// Forward declaration
void onLongPress();

// Button long press tracking
unsigned long buttonPressStart = 0;
bool buttonHeld = false;
bool wifiStarted = false;

// External global instances
extern SDLogger sdLogger;

File dataFile;
std::vector<String> serviceUuids;

void setup() {
  M5.Power.begin();
  M5Cardputer.begin();
  Serial.begin(115200);
  delay(500);

  Serial.println("GhostBLE starting...");

  initLogger();

  #if defined(CARDPUTER)
  M5.Lcd.setRotation(1);
  #endif

  M5.Lcd.fillScreen(BLACK);
  //M5.Lcd.drawBmp(nibblesStartWorking, sizeof(nibblesStartWorking));
  delay(250);

  NimBLEDevice::init("bleDefender");
  Serial.println("BLE initialized successfully.");

  drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
  drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
  drawOverlay(speechBubble, SPEECHBUBBLE_WIDTH, SPEECHBUBBLE_HEIGHT, 130, 15);
  delay(200);


  #if defined(CARDPUTER)
  if (!sdLogger.begin(SD_CS_PIN)) {
    M5.Lcd.setTextColor(BLACK); 
    M5.Lcd.setTextSize(1); 
    M5.Lcd.setCursor(140, 27);
    M5.Lcd.println("  NO SD CARD! ");
    while (1);
  }
  #endif

  M5.Lcd.setTextColor(BLACK); 
  M5.Lcd.setTextSize(1); 
  M5.Lcd.setCursor(140, 27);
  M5.Lcd.println(" HI I'M NIBBLES");
  vTaskDelay(pdMS_TO_TICKS(2000));  // 3 Sekunden

  // **Hier den Webserver und AP direkt starten**
  isWebLogActive = true;     // Status anpassen
  startWebLogServer();

  ws.textAll("BLE_SCAN_READY");

  startTimeDevice = millis();
  scanIsRunning = false;

  //M5.Speaker.tone(1800, 120);
  //playMysteryBoot();

  delay(3000);
  //playNotificationPro();

  toggleWiFi();

  // To Update Wifi Logo to ON
  showFindingCounter(targetConnects, susDevice, leakedCounter);
}


void loop() {
  ws.cleanupClients();  // Wichtig für WebSocket-Verbindungen
  M5Cardputer.update();
  unsigned long currentTime = millis();

  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      auto status = M5Cardputer.Keyboard.keysState();

      /*
      for (char c : status.word) {
        Serial.printf("Key: %c\n", c);

        if (c == 'w' || c == 'W') {
          toggleWiFi();
        }
      }
      */

      if (status.enter) {
        Serial.println("ENTER pressed");
      } 
      if (status.fn) {    
        Serial.println("FN pressed");
        toggleWiFi();
      }
      if (status.tab){
        Serial.println("TAB pressed");
      }   
      if (status.del){
        Serial.println("DEL pressed");
      }   
    }
  }

  // Long press detection for BtnG0
  if (M5Cardputer.BtnA.isPressed()) {
    //Serial.println("### BUTTON BTN0 PRESSED ###");
    if (!buttonHeld) {
      if (buttonPressStart == 0) {
        buttonPressStart = currentTime;
      } else if (currentTime - buttonPressStart >= 1000) {  // 1 second
        buttonHeld = true;
        onLongPress();  // Call your custom function
      }
    }
  } else {
    buttonPressStart = 0;
    buttonHeld = false;
  }

  // BLE scan loop
  if (bleScanEnabledWeb) {
    if (currentTime - lastFaceUpdate > FACE_UPDATE_INTERVAL_MS) {
      if (!targetFound && !scanIsRunning) {
        scanForDevices();
      } else {
        targetFound = false;
      }
      lastFaceUpdate = currentTime;
    }
  }

  // Timer every 60 minutes
  unsigned long currentTimeDevice = millis();
  if (currentTimeDevice - startTimeDevice >= timerDurationDevice) {
    Serial.println("60 Minuten sind vorbei!");
    if (!seenDevices.empty()) {
      seenDevices.clear();
      Serial.println("CLEAR SEEN DEVICES");
    } else {
      Serial.println("SEEN DEVICES STILL EMPTY");
    }
    Serial.println("Update Batterie State");
    startTimeDevice = millis();
  }
  // Let system handle BLE, GPIO, etc.
  yield();
}

void onLongPress() {
  bleScanEnabledWeb = !bleScanEnabledWeb;

  if (bleScanEnabledWeb) {
    logToSerialAndWeb("▶️ BLE Scan ENABLED");
    ws.textAll("BLE_SCAN_ON");
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesThugLife, NIBBLESTHUGLIFE_WIDTH, NIBBLESTHUGLIFE_HEIGHT, 80, 52);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice); 
  } 
  else {
    logToSerialAndWeb("⏹️ BLE Scan DISABLED");
    ws.textAll("BLE_SCAN_OFF");
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesSad, NIBBLESSAD_WIDTH, NIBBLESSAD_HEIGHT, 83, 56);
    showFindingCounter(targetConnects, susDevice, allSpottedDevice); 
    stopBleScan();   // 👈 THIS is the important part
  }
}

void toggleWiFi() {
  if (wifiStarted) {
    logToSerialAndWeb("WIFI / WEB SERVER OFF");
    stopWebLogServer();
    wifiStarted = false;
    isWebLogActive = false;
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, leakedCounter); // optional: Icon OFF
  } else {
    logToSerialAndWeb("WIFI / WEB SERVER ON");
    startWebLogServer();
    isWebLogActive = true;
    drawOverlay(nibblesFront, NIBBLESFRONT_WIDTH, NIBBLESFRONT_HEIGHT, 5, 0);
    drawOverlay(nibblesHappy, NIBBLESHAPPY_WIDTH, NIBBLESHAPPY_HEIGHT, 83, 60);
    showFindingCounter(targetConnects, susDevice, leakedCounter); // optional: Icon ON
  }

  //playNotificationPro(); // optional akustisches Feedback
}

void stopWebLogServer() {
  ws.closeAll();       // alle Clients trennen
  server.end();        // Server stoppen
  delay(50); 
  WiFi.softAPdisconnect(true);  // Access Point beenden
  delay(50);

  WiFi.mode(WIFI_OFF);
  delay(100);                   // 👈 THIS is critical

  wifiStarted    = false;
  isWebLogActive = false;

  Serial.println("WiFi fully stopped");
}


void startWebLogServer() {
  logToSerialAndWeb("WEB SERVER");
  if (wifiStarted) return;  // Don't start twice
  WiFi.mode(WIFI_AP);
  Serial.print("   - SoftAP started? YES\n");
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("   - Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // Setup WebSocket events and handlers BEFORE starting the server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
  logToSerialAndWeb("   - Web server started.");
  wifiStarted = true;

  logToSerialAndWeb("Pres BtnG0 to TOGGLE BLE Scan");
}

