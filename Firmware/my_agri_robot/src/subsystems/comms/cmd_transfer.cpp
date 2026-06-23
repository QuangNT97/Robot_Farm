#include "cmd_transfer.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cmd_transfer, LOG_LEVEL_DBG);

/* Response opcodes */
#define RESP_STATUS     0x80U
#define RESP_ACK        0x81U
#define RESP_FAULT      0x82U

CmdTransfer::CmdTransfer(ICmdHW *hw) : m_hw(hw) {}

int CmdTransfer::SendStatus(MotorID_t motorId, MotorState_t state, MotorSpeed_t speedHz)
{
    /* Pack: dataHi = state, dataLo = speed clamped to 8-bit */
    uint8_t dataHi = static_cast<uint8_t>(state);
    uint8_t dataLo = static_cast<uint8_t>(speedHz > 0xFFU ? 0xFFU : speedHz);
    return BuildAndSend(motorId, RESP_STATUS, dataHi, dataLo);
}

int CmdTransfer::SendAck(MotorID_t motorId, uint8_t opcode)
{
    return BuildAndSend(motorId, RESP_ACK, opcode, 0x00U);
}

int CmdTransfer::SendFault(MotorID_t motorId, uint8_t faultCode)
{
    return BuildAndSend(motorId, RESP_FAULT, faultCode, 0x00U);
}

int CmdTransfer::BuildAndSend(MotorID_t motorId, uint8_t opcode,
                               uint8_t dataHi, uint8_t dataLo)
{
    m_txBuf[FRAME_IDX_HEADER]   = FRAME_HEADER;
    m_txBuf[FRAME_IDX_MOTOR_ID] = motorId;
    m_txBuf[FRAME_IDX_OPCODE]   = opcode;
    m_txBuf[FRAME_IDX_DATA_HI]  = dataHi;
    m_txBuf[FRAME_IDX_DATA_LO]  = dataLo;
    m_txBuf[FRAME_IDX_CRC]      = ComputeCRC(motorId, opcode, dataHi, dataLo);
    m_txBuf[FRAME_IDX_END]      = FRAME_END;

    LOG_DBG("TX: [%02X %02X %02X %02X %02X %02X %02X]",
            m_txBuf[0], m_txBuf[1], m_txBuf[2],
            m_txBuf[3], m_txBuf[4], m_txBuf[5], m_txBuf[6]);

    return m_hw->Transmit(m_txBuf, FRAME_SIZE);
}

uint8_t CmdTransfer::ComputeCRC(uint8_t motorId, uint8_t opcode,
                                  uint8_t dataHi, uint8_t dataLo)
{
    return motorId ^ opcode ^ dataHi ^ dataLo;
}
