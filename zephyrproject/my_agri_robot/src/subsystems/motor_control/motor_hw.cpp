#include "motor_hw.h"
#include "app_config.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(motor_hw, LOG_LEVEL_DBG);

/*
 * DT aliases bound in app.overlay:
 *   pwm1        -> TIM1_CH1 on PA8
 *   motor_dir   -> PE2
 *   motor_ena   -> PE3
 *   motor_alm   -> PE4 (interrupt input, ALM from CL57C)
 */
#define MOTOR_PWM_NODE  DT_NODELABEL(pwm1)
#define MOTOR_DIR_NODE  DT_PATH(motor_gpios, motor_dir)
#define MOTOR_ENA_NODE  DT_PATH(motor_gpios, motor_ena)
#define MOTOR_ALM_NODE  DT_PATH(motor_gpios, motor_alm)

/* PWM channel 1, flags 0 (normal polarity) */
#define MOTOR_PWM_CHANNEL   1U
#define MOTOR_PWM_FLAGS     PWM_POLARITY_NORMAL

/* Global pointer for ISR -> MotorHW back-link (one motor instance only) */
static MotorHW *g_MotorHWInstance = nullptr;

int MotorHW::Init()
{
    /* --- PWM (PA8 / TIM1_CH1) --- */
    m_pwmSpec = PWM_DT_SPEC_GET_BY_IDX(DT_NODELABEL(pwm1), 0);
    if (!device_is_ready(m_pwmSpec.dev)) {
        LOG_ERR("PWM device not ready");
        return -ENODEV;
    }

    /* --- DIR pin (PE2) --- */
    m_dirSpec = GPIO_DT_SPEC_GET(DT_PATH(motor_gpios, motor_dir), gpios);
    if (!device_is_ready(m_dirSpec.port)) {
        LOG_ERR("DIR GPIO not ready");
        return -ENODEV;
    }
    if (gpio_pin_configure_dt(&m_dirSpec, GPIO_OUTPUT_INACTIVE) < 0) {
        LOG_ERR("DIR GPIO configure failed");
        return -EIO;
    }

    /* --- ENA pin (PE3) --- */
    m_enaSpec = GPIO_DT_SPEC_GET(DT_PATH(motor_gpios, motor_ena), gpios);
    if (!device_is_ready(m_enaSpec.port)) {
        LOG_ERR("ENA GPIO not ready");
        return -ENODEV;
    }
    if (gpio_pin_configure_dt(&m_enaSpec, GPIO_OUTPUT_INACTIVE) < 0) {
        LOG_ERR("ENA GPIO configure failed");
        return -EIO;
    }

    /* --- ALM pin (PE4) - interrupt input --- */
    m_almSpec = GPIO_DT_SPEC_GET(DT_PATH(motor_gpios, motor_alm), gpios);
    if (!device_is_ready(m_almSpec.port)) {
        LOG_ERR("ALM GPIO not ready");
        return -ENODEV;
    }
    if (gpio_pin_configure_dt(&m_almSpec, GPIO_INPUT) < 0) {
        LOG_ERR("ALM GPIO configure failed");
        return -EIO;
    }

    /* Configure rising-edge interrupt for ALM */
    gpio_init_callback(&m_almCb, AlmIsrHandler, BIT(m_almSpec.pin));
    if (gpio_add_callback(m_almSpec.port, &m_almCb) < 0) {
        LOG_ERR("ALM GPIO callback add failed");
        return -EIO;
    }
    if (gpio_pin_interrupt_configure_dt(&m_almSpec, GPIO_INT_EDGE_RISING) < 0) {
        LOG_ERR("ALM GPIO interrupt configure failed");
        return -EIO;
    }

    /* Store back-link for ISR */
    g_MotorHWInstance = this;

    m_initialized = true;
    LOG_INF("MotorHW initialized: PWM=PA8, DIR=PE2, ENA=PE3, ALM=PE4");
    return 0;
}

void MotorHW::SetDirection(MotorDirection_t dir)
{
    /* MOTOR_DIR_FORWARD = 0 -> PE2 LOW; MOTOR_DIR_BACKWARD = 1 -> PE2 HIGH */
    gpio_pin_set_dt(&m_dirSpec, (dir == MOTOR_DIR_BACKWARD) ? 1 : 0);
    LOG_DBG("DIR set to %s", (dir == MOTOR_DIR_BACKWARD) ? "BACKWARD" : "FORWARD");
}

void MotorHW::Enable(bool enable)
{
    /* PE3 HIGH = enable driver, PE3 LOW = disable driver */
    gpio_pin_set_dt(&m_enaSpec, enable ? 1 : 0);
    LOG_DBG("ENA set to %s", enable ? "ENABLED" : "DISABLED");
}

int MotorHW::SetPulseFrequency(MotorSpeed_t speedHz)
{
    if (speedHz == 0U) {
        StopPulse();
        return 0;
    }

    if (speedHz > MOTOR_MAX_SPEED_HZ) {
        speedHz = MOTOR_MAX_SPEED_HZ;
    }

    /* period_ns = 1_000_000_000 / speedHz */
    uint32_t periodNs = 1000000000U / speedHz;
    uint32_t pulseNs  = MOTOR_MIN_PULSE_NS;

    /* Pulse width must not exceed half-period to avoid overlap */
    if (pulseNs >= periodNs / 2U) {
        pulseNs = periodNs / 2U;
    }

    int ret = pwm_set(m_pwmSpec.dev, MOTOR_PWM_CHANNEL,
                      periodNs, pulseNs, MOTOR_PWM_FLAGS);
    if (ret < 0) {
        LOG_ERR("PWM set failed: %d", ret);
        return ret;
    }

    m_currentPeriodNs = periodNs;
    LOG_DBG("PWM: speed=%u Hz, period=%u ns, pulse=%u ns", speedHz, periodNs, pulseNs);
    return 0;
}

void MotorHW::StopPulse()
{
    /* Set pulse width to 0 to stop generating pulses */
    pwm_set(m_pwmSpec.dev, MOTOR_PWM_CHANNEL,
            m_currentPeriodNs > 0U ? m_currentPeriodNs : 1000000U,
            0U, MOTOR_PWM_FLAGS);
    m_currentPeriodNs = 0U;
    LOG_DBG("PWM stopped");
}

void MotorHW::RegisterAlarmCallback(AlarmCallback_t cb, void *ctx)
{
    m_alarmCb  = cb;
    m_alarmCtx = ctx;
}

bool MotorHW::HealthCheck()
{
    if (!m_initialized) {
        return false;
    }
    /* Check ALM pin is not already asserted (LOW = OK on open-collector ALM) */
    int almState = gpio_pin_get_dt(&m_almSpec);
    if (almState > 0) {
        LOG_WRN("HealthCheck: ALM already asserted!");
        return false;
    }
    /* Check ENA GPIO is readable (device is ready) */
    if (!device_is_ready(m_enaSpec.port)) {
        return false;
    }
    LOG_DBG("HealthCheck: OK");
    return true;
}

/* ISR runs in interrupt context - must stay minimal */
void MotorHW::AlmIsrHandler(const struct device *port,
                              struct gpio_callback *cb,
                              gpio_port_pins_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(pins);

    if (g_MotorHWInstance && g_MotorHWInstance->m_alarmCb) {
        g_MotorHWInstance->m_alarmCb(g_MotorHWInstance->m_alarmCtx);
    }
}
