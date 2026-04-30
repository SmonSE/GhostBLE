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
//    - std::atomic<bool>  → set FreeRTOS-Tasks (Core 1)
//                           this Flags read, loop() (Core 0)
//    - plain field        → only from loop() / use setup()
//    - taskMutex          → needs to be set before every 
//                           TaskHandle access
// ============================================================

namespace UIContext {

// ------------------------------------------------------------
//  FreeRTOS-Infrastructure
//  taskMutex safes all TaskHandle-Operations
// ------------------------------------------------------------
extern SemaphoreHandle_t taskMutex;

// ------------------------------------------------------------
//  Animations-Task-Handles
//  Before access use taskMutex!
// ------------------------------------------------------------
extern TaskHandle_t glassesTaskHandle;
extern TaskHandle_t angryTaskHandle;
extern TaskHandle_t happyTaskHandle;
extern TaskHandle_t sadTaskHandle;

// ------------------------------------------------------------
//  Animations-Flags  (Core 1 write ↔ Core 0 read → atomic)
// ------------------------------------------------------------
extern std::atomic<bool> isGlassesTaskRunning;
extern std::atomic<bool> isAngryTaskRunning;
extern std::atomic<bool> isSadTaskRunning;
extern std::atomic<bool> isHappyTaskRunning;
extern std::atomic<bool> isThugLifeTaskRunning;
extern std::atomic<bool> isSpeechBubbleActive;

extern std::atomic<bool> isEvilModeActive;

// ------------------------------------------------------------
//  Overlay-State  (nur loop() read/write)
// ------------------------------------------------------------
extern bool helpOverlayVisible;

// ------------------------------------------------------------
//  Show Device-State  (only loop() read/write)
// ------------------------------------------------------------
extern std::atomic<bool> isChargingState;
extern std::atomic<int>  batteryPercent;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------

// Initialisations taskMutex. Called once in setup(),
// bevore any task is running.
void init();

// Gave true back if any Expression-Animation is running.
// Usefull if competing Animations-Tasks.
bool isAnyExpressionRunning();

// Stopps running Task gracefully (check Handle + Flag).
// expression: Pointer at atomic<bool>-Flag of Tasks
// handle:     Pointer at associated TaskHandle_t
void stopExpressionTask(std::atomic<bool>& runningFlag,
                        TaskHandle_t&      handle);

} // namespace UIContext
