#include "motor_app.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_app, LOG_LEVEL_DBG);

MotorApp::MotorApp(MotorDrv *drv, MotorSM *sm) : m_drv(drv), m_sm(sm) {}

void MotorApp::ResetTelemetry()
{
    m_faultCount  = 0U;
    m_totalRunMs  = 0U;
    m_runStartMs  = 0U;
    m_almAsserted = false;
    m_running     = false;
    LOG_DBG("MotorApp: telemetry reset");
}

void MotorApp::NotifyAlarm()
{
    /* Called from ISR - no logging, no blocking */
    m_faultCount++;
    m_almAsserted = true;
}

void MotorApp::NotifyRunStart()
{
    m_runStartMs = k_uptime_get_32();
    m_running    = true;
    LOG_DBG("MotorApp: run started at %u ms", m_runStartMs);
}

void MotorApp::NotifyRunStop()
{
    if (m_running) {
        uint32_t elapsed = k_uptime_get_32() - m_runStartMs;
        m_totalRunMs += elapsed;
        m_running = false;
        LOG_DBG("MotorApp: run stopped, elapsed=%u ms, total=%u ms", elapsed, m_totalRunMs);
    }
}

bool MotorApp::CheckFaults()
{
    if (m_almAsserted) {
        LOG_WRN("MotorApp: ALM fault detected (count=%u)", m_faultCount);
        m_almAsserted = false;
        return true;
    }
    return false;
}

void MotorApp::GetTelemetry(Telemetry &out) const
{
    out.state       = m_sm->GetState();
    out.direction   = m_drv->GetCurrentDirection();
    out.speedHz     = m_drv->GetCurrentSpeed();
    out.faultCount  = m_faultCount;
    out.runTimeMs   = m_totalRunMs;
    out.almAsserted = m_almAsserted;
}
