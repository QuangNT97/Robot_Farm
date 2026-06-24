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
     * @param seqId    Sequence ID echoed from received command.
     * @param motorId  Motor driver ID.
     * @param state    Current motor state.
     * @param speedHz  Current speed in Hz.
     * @return 0 on success, negative errno on failure.
     */
    int SendStatus(uint8_t seqId, MotorID_t motorId, MotorState_t state, MotorSpeed_t speedHz);

    /**
     * @brief Send an ACK frame acknowledging a received command.
     * @param seqId    Sequence ID echoed from received command.
     * @param motorId  Motor driver ID.
     * @param opcode   The opcode being acknowledged.
     * @return 0 on success, negative errno on failure.
     */
    int SendAck(uint8_t seqId, MotorID_t motorId, uint8_t opcode);

    /**
     * @brief Send a fault notification frame to master.
     * @param seqId      Sequence ID echoed from received command.
     * @param motorId    Motor driver ID.
     * @param faultCode  Fault reason code.
     * @return 0 on success, negative errno on failure.
     */
    int SendFault(uint8_t seqId, MotorID_t motorId, uint8_t faultCode);

    /**
     * @brief Send unsolicited 10-byte notify frame to master (slave → master).
     *        Opcode is STATUS (0x80) when faultCode==0, FAULT (0x82) otherwise.
     * @param txSeqId  Slave-owned sequence ID (incremented by caller per send).
     * @param notify   Motor notify data (state, speedRPM, faultCode).
     * @return 0 on success, negative errno on failure.
     */
    int SendNotify(uint8_t txSeqId, const MotorNotify_t &notify);

private:
    ICmdHW *m_hw;

    /* Static TX buffer for 8-byte command responses (ACK/FAULT) */
    uint8_t m_txBuf[FRAME_SIZE] {};

    /* Static TX buffer for 10-byte notify frames (STATUS/FAULT notify) */
    uint8_t m_notifyTxBuf[NOTIFY_FRAME_SIZE] {};

    /**
     * @brief Build a frame in m_txBuf and transmit it.
     * @param seqId    Sequence ID to embed in response frame.
     * @param motorId  Motor ID.
     * @param opcode   Response opcode.
     * @param dataHi   High byte of data.
     * @param dataLo   Low byte of data.
     * @return 0 on success, negative errno on failure.
     */
    int BuildAndSend(uint8_t seqId, MotorID_t motorId, uint8_t opcode,
                     uint8_t dataHi, uint8_t dataLo);

    /** Compute XOR CRC over fields SeqID, MotorID, OP, DataHI, DataLO */
    static uint8_t ComputeCRC(uint8_t seqId, uint8_t motorId, uint8_t opcode,
                               uint8_t dataHi, uint8_t dataLo);

    /** Compute XOR CRC over all notify payload bytes [1]..[7] */
    static uint8_t ComputeNotifyCRC(uint8_t txSeqId, uint8_t motorId, uint8_t opcode,
                                    uint8_t state, uint16_t speedRPM, uint8_t faultCode);
};
