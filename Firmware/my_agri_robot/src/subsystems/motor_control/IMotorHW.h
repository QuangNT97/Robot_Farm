#pragma once

#include "motor_interface.h"
#include <stdint.h>

/**
 * @brief Pure abstract interface between motor_drv and hardware.
 *        motor_drv depends ONLY on this interface, never on motor_hw.h directly.
 *        Enables unit-testing motor_drv with a mock hardware implementation.
 */
class IMotorHW {
public:
    /** Callback type invoked from ISR when CL57C ALM signal is asserted */
    using AlarmCallback_t = void (*)(void *ctx);

    virtual ~IMotorHW() = default;

    /**
     * @brief Initialize hardware peripherals (PWM, GPIO, ISR).
     * @return 0 on success, negative errno on failure.
     */
    virtual int Init() = 0;

    /**
     * @brief Set motor rotation direction by driving PE2.
     *        Must only be called when speed == 0.
     * @param dir MOTOR_DIR_FORWARD or MOTOR_DIR_BACKWARD.
     */
    virtual void SetDirection(MotorDirection_t dir) = 0;

    /**
     * @brief Enable or disable the CL57C driver via PE3.
     *        PE3 HIGH = enable, PE3 LOW = disable.
     * @param enable true to enable, false to disable.
     */
    virtual void Enable(bool enable) = 0;

    /**
     * @brief Set step pulse frequency on PA8 (TIM1 PWM).
     *        Frequency range: 0 (stop) to MOTOR_MAX_SPEED_HZ.
     * @param speedHz Pulse frequency in Hz. 0 stops the PWM output.
     * @return 0 on success, negative errno on failure.
     */
    virtual int SetPulseFrequency(MotorSpeed_t speedHz) = 0;

    /**
     * @brief Immediately stop pulse output (emergency stop).
     *        Does not disable the driver (ENA stays HIGH).
     */
    virtual void StopPulse() = 0;

    /**
     * @brief Register a callback to be notified when ALM signal fires.
     *        The callback is invoked from ISR context - keep it minimal.
     * @param cb   Function pointer to the alarm handler.
     * @param ctx  User context pointer passed to cb.
     */
    virtual void RegisterAlarmCallback(AlarmCallback_t cb, void *ctx) = 0;

    /**
     * @brief Perform a health check: verify driver is responsive and supply is OK.
     * @return true if driver is healthy, false otherwise.
     */
    virtual bool HealthCheck() = 0;
};
