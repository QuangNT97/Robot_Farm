#include "cmd_task.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(cmd_task, LOG_LEVEL_DBG);

CmdTask *CmdTask::s_instance = nullptr;

CmdTask::CmdTask(MotorTask *motorTask)
    : m_motorTask(motorTask)
    , m_hw()
    , m_receiver(&m_hw)
    , m_transfer(&m_hw)
    , m_parser()
{
    s_instance = this;
}

int CmdTask::Init()
{
    /* Initialize frame queue (interrupt -> task deferred processing) */
    k_msgq_init(&m_frameQueue,
                 reinterpret_cast<char *>(m_frameQueueBuf),
                 FRAME_SIZE,
                 CMD_MSG_QUEUE_DEPTH);

    /* Initialize hardware (UART async + ring buffer) */
    int ret = m_hw.Init();
    if (ret < 0) {
        LOG_ERR("CmdHW init failed: %d", ret);
        return ret;
    }

    /* Register frame-complete callback */
    m_receiver.RegisterFrameCallback(OnFrameReady, this);

    /* Start receiver (registers byte callback with CmdHW) */
    m_receiver.Init();

    LOG_INF("CmdTask initialized");
    return 0;
}

void CmdTask::Run()
{
    uint8_t frame[FRAME_SIZE];

    while (true) {
        /* Block waiting for a complete frame from the interrupt handler */
        int ret = k_msgq_get(&m_frameQueue, frame, K_FOREVER);
        if (ret == 0) {
            HandleFrame(frame, FRAME_SIZE);
        }
    }
}

void CmdTask::OnFrameReady(const uint8_t *frame, uint8_t len, void *ctx)
{
    /* Called from ISR context - only enqueue, no heavy work */
    CmdTask *self = static_cast<CmdTask *>(ctx);
    if (len != FRAME_SIZE) {
        return;
    }
    /* k_msgq_put is ISR-safe */
    k_msgq_put(&self->m_frameQueue, frame, K_NO_WAIT);
}

void CmdTask::HandleFrame(const uint8_t *frame, uint8_t len)
{
    MotorMessage_t msg {};
    if (!m_parser.Parse(frame, len, msg)) {
        LOG_WRN("CmdTask: frame parse failed");
        return;
    }

    /* SPE opcode parser sets direction=FORWARD by default.
     * Override with last known direction so DIR state is preserved. */
    if (frame[FRAME_IDX_OPCODE] == OPCODE_SPE) {
        msg.Direction = m_lastDirection;
    }
    /* Update last known direction when DIR command is received */
    if (frame[FRAME_IDX_OPCODE] == OPCODE_DIR) {
        m_lastDirection = msg.Direction;
    }

    LOG_DBG("CmdTask: parsed id=%u state=%d speed=%u dir=%d",
            msg.ID, msg.State, msg.Speed, msg.Direction);

    /* Post to motor_task - non-blocking (drop if queue full) */
    int ret = m_motorTask->PostMessage(msg);
    if (ret < 0) {
        LOG_WRN("CmdTask: motor_task queue full, message dropped");
        m_transfer.SendFault(msg.ID, 0x01U);
        return;
    }

    /* Send ACK back to master */
    m_transfer.SendAck(msg.ID, frame[FRAME_IDX_OPCODE]);
}

extern "C" void CmdTask_ThreadEntry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    CmdTask::GetInstance()->Run();
}
