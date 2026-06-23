#include "motor_task.h"
#include "cmd_task.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/*
 * Static task instances - no heap allocation.
 * Constructed at startup before main() in C++ static init order.
 */
static MotorTask g_MotorTask;
static CmdTask   g_CmdTask(&g_MotorTask);

/*
 * Static thread stacks and thread control blocks.
 * K_THREAD_DEFINE registers the thread with the Zephyr scheduler.
 * Thread functions are C-linkage trampolines defined in each task file.
 */
K_THREAD_DEFINE(motorThread,
                MOTOR_TASK_STACK_SIZE,
                MotorTask_ThreadEntry,
                NULL, NULL, NULL,
                MOTOR_TASK_PRIORITY,
                0,
                0);

K_THREAD_DEFINE(cmdThread,
                CMD_TASK_STACK_SIZE,
                CmdTask_ThreadEntry,
                NULL, NULL, NULL,
                CMD_TASK_PRIORITY,
                0,
                0);

int main(void)
{
    LOG_INF("Agriculture Robot starting...");

    int ret = g_MotorTask.Init();
    if (ret < 0) {
        LOG_ERR("MotorTask init failed: %d", ret);
        return ret;
    }

    ret = g_CmdTask.Init();
    if (ret < 0) {
        LOG_ERR("CmdTask init failed: %d", ret);
        return ret;
    }

    LOG_INF("All tasks initialized - system running");
    return 0;
}
