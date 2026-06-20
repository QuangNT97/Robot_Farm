#pragma once

#include "motor_interface.h"
#include "motor_hw.h"
#include "motor_drv.h"
#include "motor_sm.h"
#include "motor_app.h"

#include <zephyr/kernel.h>

/**
 * @brief Motor task: coordinates motor_hw, motor_drv, motor_sm, motor_app.
 *        - Receives MotorMessage_t from message queue (posted by cmd_task)
 *        - Translates messages to state machine events
 *        - Handles ALM (alarm) events from motor_hw ISR via k_event
 *        - Reports status back via transfer queue
 */
class MotorTask {
public:
    MotorTask();
    ~MotorTask() = default;

    /**
     * @brief Initialize all motor subsystem components.
     *        Must be called before Start().
     * @return 0 on success, negative errno on failure.
     */
    int Init();

    /**
     * @brief Post a MotorMessage_t to the motor task queue.
     *        Called by cmd_task from a different thread.
     * @param msg  Message to post.
     * @return 0 on success, -EAGAIN if queue full.
     */
    int PostMessage(const MotorMessage_t &msg);

    /**
     * @brief Thread entry function (registered with K_THREAD_DEFINE).
     *        Loops forever reading the message queue and driving the SM.
     */
    void Run();

    /** @brief Get pointer to singleton instance (for thread trampoline). */
    static MotorTask *GetInstance() { return s_instance; }

private:
    MotorHW  m_hw;
    MotorDrv m_drv;
    MotorSM  m_sm;
    MotorApp m_app;

    /* Message queue: cmd_task -> motor_task */
    struct k_msgq m_msgq;
    uint8_t       m_msgqBuf[MOTOR_MSG_QUEUE_DEPTH * sizeof(MotorMessage_t)];

    /* Event flag for ALM signal (from ISR -> task) */
    struct k_event m_events;
    static constexpr uint32_t EVT_ALM = BIT(0);

    static MotorTask *s_instance;

    /* ISR -> task notification for alarm */
    static void OnAlarmISR(void *ctx);

    /* State machine state-change callback */
    static void OnStateChange(MotorState_t newState, void *ctx);

    /** Translate MotorMessage_t into a MotorEvent_t and feed to SM. */
    void DispatchMessage(const MotorMessage_t &msg);
};

/* C-linkage trampoline for K_THREAD_DEFINE */
extern "C" void MotorTask_ThreadEntry(void *p1, void *p2, void *p3);
