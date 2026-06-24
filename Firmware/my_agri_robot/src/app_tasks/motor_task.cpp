#include "motor_task.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_task, LOG_LEVEL_DBG);

MotorTask *MotorTask::s_instance = nullptr;

MotorTask::MotorTask()
    : m_hw()
    , m_drv(&m_hw)
    , m_sm(&m_drv)
    , m_app(&m_drv, &m_sm)
{
    s_instance = this;
}

int MotorTask::Init()
{
    /* Initialize message queue with static buffer (no malloc) */
    k_msgq_init(&m_msgq, reinterpret_cast<char *>(m_msgqBuf),
                 sizeof(MotorMessage_t), MOTOR_MSG_QUEUE_DEPTH);

    /* Initialize notify queue (motor_task -> cmd_task direction) */
    k_msgq_init(&m_notifyQueue, reinterpret_cast<char *>(m_notifyQueueBuf),
                 sizeof(MotorNotify_t), NOTIFY_MSG_QUEUE_DEPTH);

    /* Initialize event flags for ALM notification */
    k_event_init(&m_events);

    /* Initialize driver (init hw internally) */
    int ret = m_drv.Init();
    if (ret < 0) {
        LOG_ERR("MotorDrv init failed: %d", ret);
        return ret;
    }

    /* Register ALM ISR -> task notification */
    m_drv.RegisterAlarmCallback(OnAlarmISR, this);

    /* Register SM state change callback */
    m_sm.RegisterStateChangeCallback(OnStateChange, this);

    /* Enter state machine INIT state */
    m_sm.Init();

    /* Run health check and trigger SM transition */
    bool healthy = m_drv.HealthCheck();
    if (!healthy) {
        m_lastFaultCode = FAULT_CODE_HEALTH_FAIL;
    }
    m_sm.ProcessEvent(healthy ? MOTOR_EVT_INIT_OK : MOTOR_EVT_INIT_FAIL);

    LOG_INF("MotorTask initialized, state=%s", healthy ? "READY" : "FAULT");
    return 0;
}

int MotorTask::PostMessage(const MotorMessage_t &msg)
{
    return k_msgq_put(&m_msgq, &msg, K_NO_WAIT);
}

int MotorTask::ReadNotify(MotorNotify_t &notify, k_timeout_t timeout)
{
    return k_msgq_get(&m_notifyQueue, &notify, timeout);
}

void MotorTask::Run()
{
    MotorMessage_t msg;

    while (true) {
        /* Check for ALM event (non-blocking) */
        uint32_t events = k_event_wait(&m_events, EVT_ALM, false, K_NO_WAIT);
        if (events & EVT_ALM) {
            k_event_clear(&m_events, EVT_ALM);
            LOG_WRN("MotorTask: ALM event received");
            m_app.NotifyAlarm();
            m_lastFaultCode = FAULT_CODE_ALM;
            m_sm.ProcessEvent(MOTOR_EVT_FAULT);
        }

        /* Block on message queue with timeout to also check ALM periodically */
        int ret = k_msgq_get(&m_msgq, &msg, K_MSEC(100));
        if (ret == 0) {
            DispatchMessage(msg);
        } else if (ret == -EAGAIN) {
            /* Timeout - check faults periodically */
            if (m_app.CheckFaults()) {
                m_lastFaultCode = FAULT_CODE_ALM;
                m_sm.ProcessEvent(MOTOR_EVT_FAULT);
            }
        }
    }
}

void MotorTask::DispatchMessage(const MotorMessage_t &msg)
{
    LOG_DBG("MotorTask: msg state=%d speed=%u dir=%d", msg.State, msg.Speed, msg.Direction);

    switch (msg.State) {
    case MOTOR_STATE_RUNNING:
        m_app.NotifyRunStart();
        m_sm.ProcessEvent(MOTOR_EVT_CMD_RUN, &msg);
        break;

    case MOTOR_STATE_STOP:
        m_app.NotifyRunStop();
        m_sm.ProcessEvent(MOTOR_EVT_CMD_STOP, &msg);
        break;

    case MOTOR_STATE_INIT:
        /* Reset command from master */
        m_sm.ProcessEvent(MOTOR_EVT_RESET);
        m_app.ResetTelemetry();
        /* Re-run health check after reset */
        {
            bool healthy = m_drv.HealthCheck();
            m_sm.ProcessEvent(healthy ? MOTOR_EVT_INIT_OK : MOTOR_EVT_INIT_FAIL);
        }
        break;

    default:
        LOG_WRN("MotorTask: unhandled message state %d", msg.State);
        break;
    }
}

void MotorTask::OnAlarmISR(void *ctx)
{
    MotorTask *self = static_cast<MotorTask *>(ctx);
    /* Post ALM event to task - safe from ISR context */
    k_event_post(&self->m_events, EVT_ALM);
}

void MotorTask::OnStateChange(MotorState_t newState, void *ctx)
{
    MotorTask *self = static_cast<MotorTask *>(ctx);

    /* Convert Hz -> RPM for the frame (master unit) */
    MotorSpeed_t speedHz = self->m_drv.GetCurrentSpeed();
    uint16_t speedRPM = static_cast<uint16_t>(
        (speedHz * 60U) / MOTOR_PULSES_PER_REV);

    MotorNotify_t notify {};
    notify.ID        = MOTOR_ID_DEFAULT;
    notify.state     = newState;
    notify.speedRPM  = speedRPM;
    notify.faultCode = (newState == MOTOR_STATE_FAULT)
                       ? self->m_lastFaultCode
                       : FAULT_CODE_NONE;

    int ret = k_msgq_put(&self->m_notifyQueue, &notify, K_NO_WAIT);
    if (ret < 0) {
        LOG_WRN("MotorTask: notify queue full, state=%d dropped", (int)newState);
    }
}

/* C-linkage trampoline for K_THREAD_DEFINE */
extern "C" void MotorTask_ThreadEntry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    MotorTask::GetInstance()->Run();
}
