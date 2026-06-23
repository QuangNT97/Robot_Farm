#pragma once

/*
 * cmd_transfer.h
 * Dependency rule: only depends on ICmdHW (abstraction), NEVER on cmd_hw.h
 */

#include "ICmdHW.h"
#include "motor_interface.h"
#include "app_config.h"
#include <stdint.h>

/**
 * @brief Builds and sends frames from STM32 to master device.
 *        Constructs frames in the standard format and delegates TX to ICmdHW.
 */
class CmdTransfer {
public:
    /**
     * @param hw Pointer to ICmdHW (injected - never instantiate cmd_hw directly here).
     */
    explicit CmdTransfer(ICmdHW *hw);
    ~CmdTransfer() = default;

    /**
     * @brief Send a motor status response frame to master.
     * @param motorId  Motor driver ID.
     * @param state    Current motor state.
     * @param speedHz  Current speed in Hz.
     * @return 0 on success, negative errno on failure.
     */
    int SendStatus(MotorID_t motorId, MotorState_t state, MotorSpeed_t speedHz);

    /**
     * @brief Send an ACK frame acknowledging a received command.
     * @param motorId  Motor driver ID.
     * @param opcode   The opcode being acknowledged.
     * @return 0 on success, negative errno on failure.
     */
    int SendAck(MotorID_t motorId, uint8_t opcode);

    /**
     * @brief Send a fault notification frame to master.
     * @param motorId    Motor driver ID.
     * @param faultCode  Fault reason code.
     * @return 0 on success, negative errno on failure.
     */
    int SendFault(MotorID_t motorId, uint8_t faultCode);

private:
    ICmdHW *m_hw;

    /* Static TX buffer - reused per frame (no dynamic alloc) */
    uint8_t m_txBuf[FRAME_SIZE] {};

    /**
     * @brief Build a frame in m_txBuf and transmit it.
     * @param motorId  Motor ID.
     * @param opcode   Response opcode.
     * @param dataHi   High byte of data.
     * @param dataLo   Low byte of data.
     * @return 0 on success, negative errno on failure.
     */
    int BuildAndSend(MotorID_t motorId, uint8_t opcode, uint8_t dataHi, uint8_t dataLo);

    /** Compute XOR CRC over fields MotorID, OP, DataHI, DataLO */
    static uint8_t ComputeCRC(uint8_t motorId, uint8_t opcode,
                               uint8_t dataHi, uint8_t dataLo);
};
