#pragma once

/*
 * motor_drv.h - Motor driver layer
 * Dependency rule: ONLY depends on IMotorHW (abstraction), never on motor_hw.h
 */

#include "IMotorHW.h"
#include "motor_interface.h"
#include "app_config.h"

/**
 * @brief Motor driver: controls CL57C via the IMotorHW abstraction.
 *        Enforces direction-change safety sequence:
 *          1. Decelerate speed to 0
 *          2. Wait MOTOR_DIR_CHANGE_WAIT_MS
 *          3. Change DIR
 *          4. Wait MOTOR_DIR_SETUP_US
 *          5. Accelerate to target speed
 */
class MotorDrv {
public:
    /**
     * @brief Construct with injected hardware interface.
     * @param hw Pointer to IMotorHW implementation (motor_hw or mock).
     */
    explicit MotorDrv(IMotorHW *hw);
    ~MotorDrv() = default;

    /**
     * @brief Initialize the driver and underlying hardware.
     * @return 0 on success, negative errno on failure.
     */
    int Init();

    /**
     * @brief Run motor at given speed and direction.
     *        Handles direction-change safety sequence internally.
     * @param dir   Desired direction.
     * @param speed Desired speed in Hz.
     * @return 0 on success, negative errno on failure.
     */
    int Run(MotorDirection_t dir, MotorSpeed_t speed);

    /**
     * @brief Controlled deceleration stop.
     *        Ramps speed down to 0 then disables pulse output.
     */
    void Stop();

    /**
     * @brief Emergency stop: immediately cut pulse, disable driver.
     */
    void EmergencyStop();

    /**
     * @brief Register callback for ALM (alarm) events from hardware.
     * @param cb  Callback invoked from ISR.
     * @param ctx User context.
     */
    void RegisterAlarmCallback(IMotorHW::AlarmCallback_t cb, void *ctx);

    /**
     * @brief Run hardware health check (delegates to IMotorHW::HealthCheck).
     * @return true if healthy.
     */
    bool HealthCheck();

    /** @brief Get current speed (Hz). */
    MotorSpeed_t GetCurrentSpeed() const { return m_currentSpeed; }

    /** @brief Get current direction. */
    MotorDirection_t GetCurrentDirection() const { return m_currentDir; }

private:
    IMotorHW        *m_hw;
    MotorSpeed_t     m_currentSpeed    {0U};
    MotorSpeed_t     m_targetSpeed     {0U};
    MotorDirection_t m_currentDir      {MOTOR_DIR_FORWARD};
    MotorDirection_t m_targetDir       {MOTOR_DIR_FORWARD};
    bool             m_initialized     {false};

    /** Decelerate from current speed to 0, blocking with k_msleep steps. */
    void DecelerateToZero();

    /** Accelerate from 0 to targetSpeed in steps. */
    void AccelerateToTarget(MotorSpeed_t targetSpeed);
};
