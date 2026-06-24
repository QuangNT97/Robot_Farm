#include "cmd_parser.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cmd_parser, LOG_LEVEL_DBG);

bool CmdParser::Parse(const uint8_t *frame, uint8_t len, MotorMessage_t &msg) const
{
    if (frame == nullptr || len < FRAME_SIZE) {
        return false;
    }

    msg.ID = static_cast<MotorID_t>(frame[FRAME_IDX_MOTOR_ID]);

    uint8_t opcode = frame[FRAME_IDX_OPCODE];
    uint16_t data  = (static_cast<uint16_t>(frame[FRAME_IDX_DATA_HI]) << 8U)
                   |  static_cast<uint16_t>(frame[FRAME_IDX_DATA_LO]);

    switch (opcode) {
    case OPCODE_SPE: {
        /* Clamp RPM to software limit before conversion (architecture rule) */
        uint32_t rpm = static_cast<uint32_t>(data);
        if (rpm > MOTOR_MAX_SPEED_RPM) {
            LOG_WRN("CmdParser: RPM %u exceeds limit %u, clamped",
                    (unsigned)rpm, MOTOR_MAX_SPEED_RPM);
            rpm = MOTOR_MAX_SPEED_RPM;
        }
        /* Convert RPM to Hz: Hz = RPM x MOTOR_PULSES_PER_REV / 60 */
        msg.State     = MOTOR_STATE_RUNNING;
        msg.Speed     = rpm * MOTOR_PULSES_PER_REV / 60U;
        /* Direction unchanged - kept from last run command */
        msg.Direction = MOTOR_DIR_FORWARD;
        LOG_DBG("CmdParser: SPE id=%u rpm=%u hz=%u", msg.ID, (unsigned)rpm, msg.Speed);
        break;
    }

    case OPCODE_DIR:
        msg.State     = MOTOR_STATE_RUNNING;
        msg.Direction = (data != 0U) ? MOTOR_DIR_BACKWARD : MOTOR_DIR_FORWARD;
        msg.Speed     = 0U;  /* Direction-only command, caller preserves speed */
        LOG_DBG("CmdParser: DIR id=%u dir=%u", msg.ID, msg.Direction);
        break;

    case OPCODE_STOP:
        msg.State     = MOTOR_STATE_STOP;
        msg.Speed     = 0U;
        msg.Direction = MOTOR_DIR_FORWARD;
        LOG_DBG("CmdParser: STOP id=%u", msg.ID);
        break;

    case OPCODE_RESET:
        msg.State     = MOTOR_STATE_INIT;
        msg.Speed     = 0U;
        msg.Direction = MOTOR_DIR_FORWARD;
        LOG_DBG("CmdParser: RESET id=%u", msg.ID);
        break;

    default:
        LOG_WRN("CmdParser: unknown opcode 0x%02X", opcode);
        return false;
    }

    return true;
}
