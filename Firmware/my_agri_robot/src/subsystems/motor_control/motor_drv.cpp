#include "motor_drv.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_drv, LOG_LEVEL_DBG);

MotorDrv::MotorDrv(IMotorHW *hw) : m_hw(hw) {}

int MotorDrv::Init()
{
    if (m_hw == nullptr) {
        return -EINVAL;
    }
    int ret = m_hw->Init();
    if (ret == 0) {
        /* Start with driver disabled until health check clears */
        m_hw->Enable(false);
        m_hw->SetDirection(MOTOR_DIR_FORWARD);
        m_initialized = true;
        LOG_INF("MotorDrv initialized");
    }
    return ret;
}

int MotorDrv::Run(MotorDirection_t dir, MotorSpeed_t speed)
{
    if (!m_initialized) {
        return -EINVAL;
    }

    m_targetSpeed = speed;
    m_targetDir   = dir;

    /* Direction change safety sequence (architecture critical rule):
     *   NEVER change DIR while SPEED > 0
     */
    if (dir != m_currentDir && m_currentSpeed > 0U) {
        LOG_DBG("Dir change requested while running - decelerating first");
        DecelerateToZero();

        /* Wait 50-100ms after stopping before changing direction */
        k_msleep(MOTOR_DIR_CHANGE_WAIT_MS);

        m_hw->SetDirection(dir);
        m_currentDir = dir;

        /* Wait minimum 5us between DIR change and first pulse */
        k_busy_wait(MOTOR_DIR_SETUP_US);
    } else if (dir != m_currentDir) {
        /* Speed is already 0, safe to change DIR immediately */
        m_hw->SetDirection(dir);
        m_currentDir = dir;
        k_busy_wait(MOTOR_DIR_SETUP_US);
    }

    /* Enable the driver if not already enabled */
    m_hw->Enable(true);

    /* Ramp to target speed (handles both acceleration and deceleration) */
    RampToTarget(speed);
    return 0;
}

void MotorDrv::Stop()
{
    if (!m_initialized) {
        return;
    }
    LOG_DBG("Controlled stop initiated");
    DecelerateToZero();
    LOG_DBG("Motor stopped");
}

void MotorDrv::EmergencyStop()
{
    /* Bypass ramp - immediate cut */
    m_hw->StopPulse();
    m_hw->Enable(false);
    m_currentSpeed = 0U;
    LOG_WRN("Emergency stop executed");
}

void MotorDrv::RegisterAlarmCallback(IMotorHW::AlarmCallback_t cb, void *ctx)
{
    m_hw->RegisterAlarmCallback(cb, ctx);
}

bool MotorDrv::HealthCheck()
{
    return m_hw->HealthCheck();
}

void MotorDrv::DecelerateToZero()
{
    RampToTarget(0U);
}

void MotorDrv::RampToTarget(MotorSpeed_t targetSpeed)
{
    if (m_currentSpeed == targetSpeed) {
        return;
    }

    if (m_currentSpeed < targetSpeed) {
        /* Acceleration ramp */
        while (m_currentSpeed < targetSpeed) {
            MotorSpeed_t remaining = targetSpeed - m_currentSpeed;
            m_currentSpeed += (remaining > MOTOR_ACCEL_STEP_HZ)
                              ? MOTOR_ACCEL_STEP_HZ : remaining;
            m_hw->SetPulseFrequency(m_currentSpeed);
            k_msleep(MOTOR_ACCEL_INTERVAL_MS);
        }
    } else {
        /* Deceleration ramp — step down toward targetSpeed, not necessarily 0 */
        while (m_currentSpeed > targetSpeed) {
            MotorSpeed_t remaining = m_currentSpeed - targetSpeed;
            m_currentSpeed -= (remaining > MOTOR_ACCEL_STEP_HZ)
                              ? MOTOR_ACCEL_STEP_HZ : remaining;
            if (m_currentSpeed == 0U) {
                m_hw->StopPulse();
            } else {
                m_hw->SetPulseFrequency(m_currentSpeed);
            }
            k_msleep(MOTOR_ACCEL_INTERVAL_MS);
        }
    }

    LOG_DBG("MotorDrv: ramp complete, speed=%u Hz", m_currentSpeed);
}
