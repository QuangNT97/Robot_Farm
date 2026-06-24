#include "cmd_transfer.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cmd_transfer, LOG_LEVEL_DBG);

/* Response opcodes */
#define RESP_STATUS     0x80U
#define RESP_ACK        0x81U
#define RESP_FAULT      0x82U

CmdTransfer::CmdTransfer(ICmdHW *hw) : m_hw(hw) {}

int CmdTransfer::SendStatus(uint8_t seqId, MotorID_t motorId,
                             MotorState_t state, MotorSpeed_t speedHz)
{
    uint8_t dataHi = static_cast<uint8_t>(state);
    uint8_t dataLo = static_cast<uint8_t>(speedHz > 0xFFU ? 0xFFU : speedHz);
    return BuildAndSend(seqId, motorId, RESP_STATUS, dataHi, dataLo);
}

int CmdTransfer::SendAck(uint8_t seqId, MotorID_t motorId, uint8_t opcode)
{
    return BuildAndSend(seqId, motorId, RESP_ACK, opcode, 0x00U);
}

int CmdTransfer::SendFault(uint8_t seqId, MotorID_t motorId, uint8_t faultCode)
{
    return BuildAndSend(seqId, motorId, RESP_FAULT, faultCode, 0x00U);
}

int CmdTransfer::BuildAndSend(uint8_t seqId, MotorID_t motorId, uint8_t opcode,
                               uint8_t dataHi, uint8_t dataLo)
{
    m_txBuf[FRAME_IDX_HEADER]   = FRAME_HEADER;
    m_txBuf[FRAME_IDX_SEQ_ID]   = seqId;
    m_txBuf[FRAME_IDX_MOTOR_ID] = motorId;
    m_txBuf[FRAME_IDX_OPCODE]   = opcode;
    m_txBuf[FRAME_IDX_DATA_HI]  = dataHi;
    m_txBuf[FRAME_IDX_DATA_LO]  = dataLo;
    m_txBuf[FRAME_IDX_CRC]      = ComputeCRC(seqId, motorId, opcode, dataHi, dataLo);
    m_txBuf[FRAME_IDX_END]      = FRAME_END;

    LOG_DBG("TX: [%02X %02X %02X %02X %02X %02X %02X %02X]",
            m_txBuf[0], m_txBuf[1], m_txBuf[2], m_txBuf[3],
            m_txBuf[4], m_txBuf[5], m_txBuf[6], m_txBuf[7]);

    return m_hw->Transmit(m_txBuf, FRAME_SIZE);
}

uint8_t CmdTransfer::ComputeCRC(uint8_t seqId, uint8_t motorId, uint8_t opcode,
                                  uint8_t dataHi, uint8_t dataLo)
{
    return seqId ^ motorId ^ opcode ^ dataHi ^ dataLo;
}

int CmdTransfer::SendNotify(uint8_t txSeqId, const MotorNotify_t &notify)
{
    uint8_t opcode = (notify.faultCode != FAULT_CODE_NONE)
                     ? NOTIFY_OPCODE_FAULT
                     : NOTIFY_OPCODE_STATUS;

    m_notifyTxBuf[NOTIFY_IDX_HEADER]     = FRAME_HEADER;
    m_notifyTxBuf[NOTIFY_IDX_SEQ_ID]     = txSeqId;
    m_notifyTxBuf[NOTIFY_IDX_MOTOR_ID]   = static_cast<uint8_t>(notify.ID);
    m_notifyTxBuf[NOTIFY_IDX_OPCODE]     = opcode;
    m_notifyTxBuf[NOTIFY_IDX_STATE]      = static_cast<uint8_t>(notify.state);
    m_notifyTxBuf[NOTIFY_IDX_SPEED_HI]   = static_cast<uint8_t>(notify.speedRPM >> 8U);
    m_notifyTxBuf[NOTIFY_IDX_SPEED_LO]   = static_cast<uint8_t>(notify.speedRPM & 0xFFU);
    m_notifyTxBuf[NOTIFY_IDX_FAULT_CODE] = notify.faultCode;
    m_notifyTxBuf[NOTIFY_IDX_CRC]        = ComputeNotifyCRC(txSeqId,
                                               static_cast<uint8_t>(notify.ID),
                                               opcode,
                                               static_cast<uint8_t>(notify.state),
                                               notify.speedRPM,
                                               notify.faultCode);
    m_notifyTxBuf[NOTIFY_IDX_END]        = FRAME_END;

    LOG_DBG("TX Notify: [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]",
            m_notifyTxBuf[0], m_notifyTxBuf[1], m_notifyTxBuf[2], m_notifyTxBuf[3],
            m_notifyTxBuf[4], m_notifyTxBuf[5], m_notifyTxBuf[6], m_notifyTxBuf[7],
            m_notifyTxBuf[8], m_notifyTxBuf[9]);

    return m_hw->Transmit(m_notifyTxBuf, NOTIFY_FRAME_SIZE);
}

uint8_t CmdTransfer::ComputeNotifyCRC(uint8_t txSeqId, uint8_t motorId, uint8_t opcode,
                                       uint8_t state, uint16_t speedRPM, uint8_t faultCode)
{
    return txSeqId ^ motorId ^ opcode ^ state
           ^ static_cast<uint8_t>(speedRPM >> 8U)
           ^ static_cast<uint8_t>(speedRPM & 0xFFU)
           ^ faultCode;
}
