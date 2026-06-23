#pragma once

#include "motor_interface.h"
#include "app_config.h"
#include <stdint.h>

/**
 * @brief Parses a raw byte frame into a MotorMessage_t interface struct.
 *        Pure stateless parser - no hardware dependency.
 *
 * Frame format: [Header][MotorID][OP][DataHI][DataLO][CRC][END]
 */
class CmdParser {
public:
    CmdParser()  = default;
    ~CmdParser() = default;

    /**
     * @brief Parse a validated frame into a MotorMessage_t.
     *        Frame must already be CRC-validated by CmdReceiver.
     *
     * @param frame Pointer to FRAME_SIZE byte array.
     * @param len   Frame length (must equal FRAME_SIZE).
     * @param[out] msg Populated MotorMessage_t on success.
     * @return true if parsed successfully, false if opcode unknown.
     */
    bool Parse(const uint8_t *frame, uint8_t len, MotorMessage_t &msg) const;
};
