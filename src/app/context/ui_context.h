#pragma once

#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// ============================================================
//  UIContext — NibBLEs-Animationen, Overlays, Display-State
//
//  Thread-safety:
//    - std::atomic<bool>  → FreeRTOS-Tasks (Core 1) setzen
//                           diese Flags, loop() (Core 0) liest
//    - plain Felder       → nur aus loop() / setup() verwendet
//    - taskMutex          → muss vor jedem TaskHandle-Zugriff
//                           gehalten werden
// ============================================================

namespace UIContext {

// ------------------------------------------------------------
//  FreeRTOS-Infrastruktur
//  taskMutex schützt alle TaskHandle-Operationen.
// ------------------------------------------------------------
extern SemaphoreHandle_t taskMutex;

// ------------------------------------------------------------
//  Animations-Task-Handles
//  Vor Zugriff immer taskMutex nehmen!
// ------------------------------------------------------------
extern TaskHandle_t glassesTaskHandle;
extern TaskHandle_t angryTaskHandle;
extern TaskHandle_t happyTaskHandle;
extern TaskHandle_t sadTaskHandle;

// ------------------------------------------------------------
//  Animations-Flags  (Core 1 schreibt ↔ Core 0 liest → atomic)
// ------------------------------------------------------------
extern std::atomic<bool> isGlassesTaskRunning;
extern std::atomic<bool> isAngryTaskRunning;
extern std::atomic<bool> isSadTaskRunning;
extern std::atomic<bool> isHappyTaskRunning;
extern std::atomic<bool> isThugLifeTaskRunning;
extern std::atomic<bool> isSpeechBubbleActive;

// ------------------------------------------------------------
//  Overlay-State  (nur loop() liest/schreibt)
// ------------------------------------------------------------
extern bool helpOverlayVisible;

// ------------------------------------------------------------
//  Gerätestatus-Anzeige  (nur loop() liest/schreibt)
// ------------------------------------------------------------
extern std::atomic<bool> isChargingState;
extern std::atomic<int>  batteryPercent;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------

// Initialisiert taskMutex. Einmalig in setup() aufrufen,
// bevor irgendein Task gestartet wird.
void init();

// Gibt true zurück wenn gerade irgendeine Expression-Animation läuft.
// Nützlich um konkurrierende Animations-Tasks zu verhindern.
bool isAnyExpressionRunning();

// Stoppt einen laufenden Task sicher (prüft Handle + Flag).
// expression: Zeiger auf das atomic<bool>-Flag des Tasks
// handle:     Zeiger auf den zugehörigen TaskHandle_t
void stopExpressionTask(std::atomic<bool>& runningFlag,
                        TaskHandle_t&      handle);

} // namespace UIContext
