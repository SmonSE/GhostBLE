#include "ui_context.h"

namespace UIContext {

// ------------------------------------------------------------
//  FreeRTOS-Infrastruktur
// ------------------------------------------------------------
SemaphoreHandle_t taskMutex = NULL;

// ------------------------------------------------------------
//  Animations-Task-Handles
// ------------------------------------------------------------
TaskHandle_t glassesTaskHandle = NULL;
TaskHandle_t angryTaskHandle   = NULL;
TaskHandle_t happyTaskHandle   = NULL;
TaskHandle_t sadTaskHandle     = NULL;

// ------------------------------------------------------------
//  Animations-Flags
// ------------------------------------------------------------
std::atomic<bool> isGlassesTaskRunning{false};
std::atomic<bool> isAngryTaskRunning{false};
std::atomic<bool> isSadTaskRunning{false};
std::atomic<bool> isHappyTaskRunning{false};
std::atomic<bool> isThugLifeTaskRunning{false};
std::atomic<bool> isSpeechBubbleActive{false};

std::atomic<bool> isEvilModeActive{false};

// ------------------------------------------------------------
//  Overlay-State
// ------------------------------------------------------------
bool helpOverlayVisible = false;

// ------------------------------------------------------------
//  Gerätestatus-Anzeige
// ------------------------------------------------------------
std::atomic<bool> isChargingState{false};
std::atomic<int>  batteryPercent{0};

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------
void init() {
    taskMutex = xSemaphoreCreateMutex();
    // Handles sind bereits NULL — kein weiterer Init nötig
}

bool isAnyExpressionRunning() {
    return isGlassesTaskRunning.load()  ||
           isAngryTaskRunning.load()    ||
           isSadTaskRunning.load()      ||
           isHappyTaskRunning.load()    ||
           isThugLifeTaskRunning.load();
}

void stopExpressionTask(std::atomic<bool>& runningFlag,
                        TaskHandle_t&      handle) {
    runningFlag.store(false);

    if (handle == NULL) return;

    if (xSemaphoreTake(taskMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        vTaskDelete(handle);
        handle = NULL;
        xSemaphoreGive(taskMutex);
    }
}

} // namespace UIContext
