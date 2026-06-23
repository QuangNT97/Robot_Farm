#include "motor_sm.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_sm, LOG_LEVEL_DBG);

MotorSM::MotorSM(MotorDrv *drv) : m_drv(drv) {}

void MotorSM::Init()
{
    m_currentState = MOTOR_STATE_INIT;
    LOG_INF("MotorSM: entering %s", StateToStr(m_currentState));
}

void MotorSM::ProcessEvent(MotorEvent_t event, const MotorMessage_t *msg)
{
    LOG_DBG("MotorSM: state=%s event=%s", StateToStr(m_currentState), EventToStr(event));

    switch (m_currentState) {
    case MOTOR_STATE_INIT:    HandleInit(event, msg);    break;
    case MOTOR_STATE_READY:   HandleReady(event, msg);   break;
    case MOTOR_STATE_RUNNING: HandleRunning(event, msg); break;
    case MOTOR_STATE_STOP:    HandleStop(event, msg);    break;
    case MOTOR_STATE_FAULT:   HandleFault(event, msg);   break;
    default:                                             break;
    }
}

void MotorSM::RegisterStateChangeCallback(StateChangeCallback_t cb, void *ctx)
{
    m_stateChangeCb  = cb;
    m_stateChangectx = ctx;
}

void MotorSM::TransitionTo(MotorState_t newState)
{
    LOG_INF("MotorSM: %s -> %s", StateToStr(m_currentState), StateToStr(newState));

    /* Architecture rule: any transition to FAULT must immediately stop motor */
    if (newState == MOTOR_STATE_FAULT) {
        m_drv->EmergencyStop();
        LOG_WRN("MotorSM: FAULT - emergency stop triggered");
    }

    m_currentState = newState;

    if (m_stateChangeCb) {
        m_stateChangeCb(newState, m_stateChangectx);
    }
}

void MotorSM::HandleInit(MotorEvent_t event, const MotorMessage_t *msg)
{
    ARG_UNUSED(msg);
    switch (event) {
    case MOTOR_EVT_INIT_OK:
        TransitionTo(MOTOR_STATE_READY);
        break;
    case MOTOR_EVT_INIT_FAIL:
        TransitionTo(MOTOR_STATE_FAULT);
        break;
    default:
        LOG_WRN("MotorSM: unexpected event %s in INIT", EventToStr(event));
        break;
    }
}

void MotorSM::HandleReady(MotorEvent_t event, const MotorMessage_t *msg)
{
    switch (event) {
    case MOTOR_EVT_CMD_RUN:
        if (msg) {
            m_lastDir   = msg->Direction;
            m_lastSpeed = msg->Speed;
            m_drv->Run(msg->Direction, msg->Speed);
        }
        TransitionTo(MOTOR_STATE_RUNNING);
        break;
    case MOTOR_EVT_FAULT:
        TransitionTo(MOTOR_STATE_FAULT);
        break;
    default:
        LOG_WRN("MotorSM: unexpected event %s in READY", EventToStr(event));
        break;
    }
}

void MotorSM::HandleRunning(MotorEvent_t event, const MotorMessage_t *msg)
{
    switch (event) {
    case MOTOR_EVT_CMD_RUN:
        /* Update speed/direction while already running */
        if (msg) {
            m_lastDir   = msg->Direction;
            m_lastSpeed = msg->Speed;
            m_drv->Run(msg->Direction, msg->Speed);
        }
        break;
    case MOTOR_EVT_CMD_STOP:
        TransitionTo(MOTOR_STATE_STOP);
        m_drv->Stop();
        TransitionTo(MOTOR_STATE_READY);
        break;
    case MOTOR_EVT_FAULT:
        TransitionTo(MOTOR_STATE_FAULT);
        break;
    default:
        LOG_WRN("MotorSM: unexpected event %s in RUNNING", EventToStr(event));
        break;
    }
}

void MotorSM::HandleStop(MotorEvent_t event, const MotorMessage_t *msg)
{
    ARG_UNUSED(msg);
    switch (event) {
    case MOTOR_EVT_STOPPED:
        TransitionTo(MOTOR_STATE_READY);
        break;
    case MOTOR_EVT_FAULT:
        TransitionTo(MOTOR_STATE_FAULT);
        break;
    default:
        LOG_WRN("MotorSM: unexpected event %s in STOP", EventToStr(event));
        break;
    }
}

void MotorSM::HandleFault(MotorEvent_t event, const MotorMessage_t *msg)
{
    ARG_UNUSED(msg);
    switch (event) {
    case MOTOR_EVT_RESET:
        /* Re-enter INIT for health check before returning to READY */
        TransitionTo(MOTOR_STATE_INIT);
        break;
    default:
        LOG_WRN("MotorSM: in FAULT - only RESET accepted, got %s", EventToStr(event));
        break;
    }
}

const char *MotorSM::StateToStr(MotorState_t state)
{
    switch (state) {
    case MOTOR_STATE_INIT:    return "INIT";
    case MOTOR_STATE_READY:   return "READY";
    case MOTOR_STATE_RUNNING: return "RUNNING";
    case MOTOR_STATE_STOP:    return "STOP";
    case MOTOR_STATE_FAULT:   return "FAULT";
    default:                  return "UNKNOWN";
    }
}

const char *MotorSM::EventToStr(MotorEvent_t event)
{
    switch (event) {
    case MOTOR_EVT_INIT_OK:   return "INIT_OK";
    case MOTOR_EVT_INIT_FAIL: return "INIT_FAIL";
    case MOTOR_EVT_CMD_RUN:   return "CMD_RUN";
    case MOTOR_EVT_CMD_STOP:  return "CMD_STOP";
    case MOTOR_EVT_STOPPED:   return "STOPPED";
    case MOTOR_EVT_FAULT:     return "FAULT";
    case MOTOR_EVT_RESET:     return "RESET";
    default:                  return "UNKNOWN";
    }
}
