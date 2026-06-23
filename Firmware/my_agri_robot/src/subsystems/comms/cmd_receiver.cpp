#include "cmd_receiver.h"

#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(cmd_receiver, LOG_LEVEL_DBG);

CmdReceiver::CmdReceiver(ICmdHW *hw) : m_hw(hw) {}

void CmdReceiver::Init()
{
    ResetAssembly();
    m_hw->RegisterRxCallback(OnByteReceived, this);
    LOG_DBG("CmdReceiver initialized");
}

void CmdReceiver::RegisterFrameCallback(FrameCallback_t cb, void *ctx)
{
    m_frameCb    = cb;
    m_frameCbCtx = ctx;
}

void CmdReceiver::OnByteReceived(uint8_t byte, void *ctx)
{
    /* Called from interrupt context - must stay fast */
    CmdReceiver *self = static_cast<CmdReceiver *>(ctx);
    self->ProcessByte(byte);
}

void CmdReceiver::ProcessByte(uint8_t byte)
{
    switch (m_rxState) {
    case RxState::WAIT_HEADER:
        if (byte == FRAME_HEADER) {
            m_frameBuf[0] = byte;
            m_frameIdx    = 1U;
            m_rxState     = RxState::RECV_BODY;
        }
        break;

    case RxState::RECV_BODY:
        if (m_frameIdx < FRAME_SIZE) {
            m_frameBuf[m_frameIdx++] = byte;
        }

        if (m_frameIdx == FRAME_SIZE) {
            /* Check end marker */
            if (m_frameBuf[FRAME_IDX_END] != FRAME_END) {
                LOG_WRN("CmdReceiver: bad END byte 0x%02X", m_frameBuf[FRAME_IDX_END]);
                ResetAssembly();
                break;
            }

            /* Validate CRC */
            if (!ValidateCRC(m_frameBuf, FRAME_SIZE)) {
                LOG_WRN("CmdReceiver: CRC mismatch");
                ResetAssembly();
                break;
            }

            /* Complete valid frame - notify upper layer */
            if (m_frameCb) {
                m_frameCb(m_frameBuf, FRAME_SIZE, m_frameCbCtx);
            }
            ResetAssembly();
        }
        break;

    default:
        ResetAssembly();
        break;
    }
}

bool CmdReceiver::ValidateCRC(const uint8_t *frame, uint8_t len)
{
    ARG_UNUSED(len);
    /* CRC = XOR of MotorID, OP, DataHI, DataLO (bytes 1..4) */
    uint8_t crc = frame[FRAME_IDX_MOTOR_ID]
                ^ frame[FRAME_IDX_OPCODE]
                ^ frame[FRAME_IDX_DATA_HI]
                ^ frame[FRAME_IDX_DATA_LO];
    return (crc == frame[FRAME_IDX_CRC]);
}

void CmdReceiver::ResetAssembly()
{
    m_rxState  = RxState::WAIT_HEADER;
    m_frameIdx = 0U;
}
