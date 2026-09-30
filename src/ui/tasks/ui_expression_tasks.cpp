#include "ui_expression_tasks.h"

#include "infrastructure/logging/logger.h"


namespace UIExpressionTasks {

bool showIfNotRunning(
    TaskFunction_t      taskFunc,
    const char*         taskName,
    std::atomic<bool>&  runningFlag,
    TaskHandle_t&       taskHandleOut,
    uint32_t            stackSize,
    UBaseType_t         priority,
    BaseType_t          core)
{
    if (runningFlag.load()) {
        return false;
    }

    runningFlag.store(true);

    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunc, taskName, stackSize, nullptr, priority, &taskHandleOut, core);

    if (result != pdPASS) {
        LOG(LOG_SYSTEM, String("Failed to create ") + taskName + " task");
        runningFlag.store(false);
        taskHandleOut = nullptr;
        return false;
    }

    return true;
}

}  // namespace UIExpressionTasks
