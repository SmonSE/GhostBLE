#pragma once

#include <Arduino.h>
#include <atomic>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


namespace UIExpressionTasks {

bool showIfNotRunning(
    TaskFunction_t      taskFunc,
    const char*         taskName,
    std::atomic<bool>&  runningFlag,
    TaskHandle_t&       taskHandleOut,
    uint32_t            stackSize = 4096,
    UBaseType_t         priority  = 3,
    BaseType_t          core      = 1);

}  // namespace UIExpressionTasks
