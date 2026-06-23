#pragma once

#include "motor_interface.h"
#include "motor_drv.h"

/**
 * @brief Motor State Machine - State Transition Table pattern.
 *
 * States: MOTOR_INIT -> MOTOR_READY -> MOTOR_RUNNING -> MOTOR_STOP -> MOTOR_FAULT
 *
 * All state transitions are event-driven (no blocking).
 * Any transition to MOTOR_FAULT immediately triggers EmergencyStop().
 * All transitions are logged for debugging.
 */
class MotorSM {
public:
    /** Callback invoked when state changes - allows motor_task to react */
    using StateChangeCallback_t = void (*)(MotorState_t newState, void *ctx);

    /**
     * @brief Construct with injected motor driver.
     * @param drv  Pointer to MotorDrv instance.
     */
    explicit MotorSM(MotorDrv *drv);
    ~MotorSM() = default;

    /** @brief Initialize and enter MOTOR_INIT state. */
    void Init();

    /**
     * @brief Process an event and execute the appropriate state transition.
     * @param event  The event to process.
     * @param msg    Associated motor message (may be null for internal events).
     */
    void ProcessEvent(MotorEvent_t event, const MotorMessage_t *msg = nullptr);

    /** @brief Get current state. */
    MotorState_t GetState() const { return m_currentState; }

    /**
     * @brief Register a callback for state change notifications.
     * @param cb   Callback function.
     * @param ctx  User context.
     */
    void RegisterStateChangeCallback(StateChangeCallback_t cb, void *ctx);

private:
    MotorDrv            *m_drv;
    MotorState_t         m_currentState {MOTOR_STATE_INIT};
    StateChangeCallback_t m_stateChangeCb{nullptr};
    void                *m_stateChangectx{nullptr};

    /* Cached last run parameters for resume after stop */
    MotorDirection_t     m_lastDir  {MOTOR_DIR_FORWARD};
    MotorSpeed_t         m_lastSpeed{0U};

    /** Transition to a new state with logging and callback notification. */
    void TransitionTo(MotorState_t newState);

    /* State handlers */
    void HandleInit   (MotorEvent_t event, const MotorMessage_t *msg);
    void HandleReady  (MotorEvent_t event, const MotorMessage_t *msg);
    void HandleRunning(MotorEvent_t event, const MotorMessage_t *msg);
    void HandleStop   (MotorEvent_t event, const MotorMessage_t *msg);
    void HandleFault  (MotorEvent_t event, const MotorMessage_t *msg);

    /** Convert state enum to human-readable string (for logging). */
    static const char *StateToStr(MotorState_t state);
    static const char *EventToStr(MotorEvent_t event);
};
