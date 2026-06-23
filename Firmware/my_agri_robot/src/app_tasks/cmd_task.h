#pragma once

#include "motor_interface.h"
#include "cmd_hw.h"
#include "cmd_receiver.h"
#include "cmd_transfer.h"
#include "cmd_parser.h"
#include "motor_task.h"

#include <zephyr/kernel.h>

/**
 * @brief Command task: coordinates cmd_hw, cmd_receiver, cmd_parser, cmd_transfer.
 *        - Receives byte stream from master via UART interrupt
 *        - Assembles and validates frames via CmdReceiver
 *        - Parses frames into MotorMessage_t via CmdParser
 *        - Posts MotorMessage_t to motor_task message queue
 *        - Sends status/ACK/fault responses via CmdTransfer
 */
class CmdTask {
public:
    /**
     * @param motorTask  Pointer to MotorTask instance (for posting messages).
     */
    explicit CmdTask(MotorTask *motorTask);
    ~CmdTask() = default;

    /**
     * @brief Initialize all command subsystem components.
     *        Must be called before Start().
     * @return 0 on success, negative errno on failure.
     */
    int Init();

    /**
     * @brief Thread entry function.
     *        Currently lightweight - frame processing is interrupt-driven.
     *        This thread handles any deferred work (e.g., sending responses).
     */
    void Run();

    /** @brief Get pointer to singleton instance. */
    static CmdTask *GetInstance() { return s_instance; }

private:
    MotorTask   *m_motorTask;
    CmdHW        m_hw;
    CmdReceiver  m_receiver;
    CmdTransfer  m_transfer;
    CmdParser    m_parser;

    /* Internal queue: interrupt -> cmd_task for deferred frame processing */
    struct k_msgq m_frameQueue;
    uint8_t       m_frameQueueBuf[CMD_MSG_QUEUE_DEPTH * FRAME_SIZE] {};

    static CmdTask *s_instance;

    /* Frame-ready callback (from CmdReceiver, interrupt context) */
    static void OnFrameReady(const uint8_t *frame, uint8_t len, void *ctx);

    /** Process a complete frame: parse -> post to motor_task -> send ACK */
    void HandleFrame(const uint8_t *frame, uint8_t len);

    /* Per-frame staging buffer for queue transfers */
    uint8_t m_stagingFrame[FRAME_SIZE] {};

    /* Track last known direction so SPE command preserves it */
    MotorDirection_t m_lastDirection {MOTOR_DIR_FORWARD};
};

extern "C" void CmdTask_ThreadEntry(void *p1, void *p2, void *p3);
