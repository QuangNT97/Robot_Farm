#pragma once

/*
 * cmd_receiver.h
 * Dependency rule: only depends on ICmdHW (abstraction), NEVER on cmd_hw.h
 */

#include "ICmdHW.h"
#include "app_config.h"
#include <stdint.h>

/**
 * @brief Collects raw bytes from ICmdHW and assembles complete legal frames.
 *        A legal frame: [0xAA][MotorID][OP][DataHI][DataLO][CRC][0x55]
 *
 * Notifies upper layer when a complete frame is assembled.
 */
class CmdReceiver {
public:
    /** Callback invoked when a complete, CRC-valid frame is assembled */
    using FrameCallback_t = void (*)(const uint8_t *frame, uint8_t len, void *ctx);

    /**
     * @param hw Pointer to ICmdHW (injected - never instantiate cmd_hw directly here).
     */
    explicit CmdReceiver(ICmdHW *hw);
    ~CmdReceiver() = default;

    /** @brief Initialize and register byte callback with hardware. */
    void Init();

    /**
     * @brief Register callback to be notified when a full frame is ready.
     * @param cb   Callback function.
     * @param ctx  User context.
     */
    void RegisterFrameCallback(FrameCallback_t cb, void *ctx);

private:
    ICmdHW         *m_hw;
    FrameCallback_t m_frameCb    {nullptr};
    void           *m_frameCbCtx {nullptr};

    /* Frame assembly state machine */
    enum class RxState : uint8_t {
        WAIT_HEADER,
        RECV_BODY,
    };

    RxState  m_rxState  {RxState::WAIT_HEADER};
    uint8_t  m_frameBuf [FRAME_SIZE] {};
    uint8_t  m_frameIdx {0U};

    /** Called by ICmdHW for each received byte (interrupt context) */
    static void OnByteReceived(uint8_t byte, void *ctx);

    /** Process a single byte through the frame assembly state machine */
    void ProcessByte(uint8_t byte);

    /** Validate frame CRC (XOR of bytes[1..4]) == bytes[5] */
    static bool ValidateCRC(const uint8_t *frame, uint8_t len);

    /** Reset assembly state to wait for next header */
    void ResetAssembly();
};
