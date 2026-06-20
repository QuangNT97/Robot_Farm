#pragma once

#include "IMotorHW.h"
#include "motor_interface.h"
#include "app_config.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/**
 * @brief Hardware layer for CL57C step motor driver.
 *        Manages: PWM (PA8), DIR (PE2), ENA (PE3), ALM interrupt (PE4).
 *        No state-machine logic here - pure hardware commands only.
 */
class MotorHW : public IMotorHW {
public:
    MotorHW() = default;
    ~MotorHW() override = default;

    int  Init() override;
    void SetDirection(MotorDirection_t dir) override;
    void Enable(bool enable) override;
    int  SetPulseFrequency(MotorSpeed_t speedHz) override;
    void StopPulse() override;
    void RegisterAlarmCallback(AlarmCallback_t cb, void *ctx) override;
    bool HealthCheck() override;

private:
    /* GPIO/PWM device specs - populated at Init() from DT */
    struct pwm_dt_spec m_pwmSpec   {};
    struct gpio_dt_spec m_dirSpec  {};
    struct gpio_dt_spec m_enaSpec  {};
    struct gpio_dt_spec m_almSpec  {};

    /* ALM interrupt callback data */
    struct gpio_callback m_almCb   {};
    AlarmCallback_t      m_alarmCb {nullptr};
    void                *m_alarmCtx{nullptr};

    /* Track current pulse period to avoid redundant PWM writes */
    uint32_t m_currentPeriodNs{0};

    bool m_initialized{false};

    /* Static ISR trampoline - Zephyr GPIO callback must be a free function */
    static void AlmIsrHandler(const struct device *port,
                               struct gpio_callback *cb,
                               gpio_port_pins_t pins);
};
