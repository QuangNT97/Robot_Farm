#pragma once

#include "motor_interface.h"
#include "motor_drv.h"
#include "motor_sm.h"

/**
 * @brief Application-specific motor support layer.
 *        Provides telemetry collection and fault condition checking.
 *        Does NOT control the motor directly - delegates to motor_sm/motor_drv.
 */
class MotorApp {
public:
    /** Telemetry snapshot */
    struct Telemetry {
        MotorState_t    state;
        MotorDirection_t direction;
        MotorSpeed_t    speedHz;
        uint32_t        faultCount;
        uint32_t        runTimeMs;
        bool            almAsserted;
    };

    /**
     * @param drv  Pointer to MotorDrv (for speed/direction queries)
     * @param sm   Pointer to MotorSM (for state queries)
     */
    MotorApp(MotorDrv *drv, MotorSM *sm);
    ~MotorApp() = default;

    /** @brief Reset telemetry counters. */
    void ResetTelemetry();

    /**
     * @brief Notify motor_app that ALM was asserted.
     *        Called from ISR context - increments fault counter only.
     */
    void NotifyAlarm();

    /**
     * @brief Notify motor_app that motor has started running.
     *        Records start time for run-time tracking.
     */
    void NotifyRunStart();

    /**
     * @brief Notify motor_app that motor has stopped.
     *        Accumulates run time.
     */
    void NotifyRunStop();

    /**
     * @brief Check for fault conditions and report.
     * @return true if a fault condition is detected (ALM asserted).
     */
    bool CheckFaults();

    /**
     * @brief Get current telemetry snapshot.
     * @param[out] out Telemetry struct to populate.
     */
    void GetTelemetry(Telemetry &out) const;

private:
    MotorDrv    *m_drv;
    MotorSM     *m_sm;
    uint32_t     m_faultCount   {0U};
    uint32_t     m_runStartMs   {0U};
    uint32_t     m_totalRunMs   {0U};
    bool         m_almAsserted  {false};
    bool         m_running      {false};
};
