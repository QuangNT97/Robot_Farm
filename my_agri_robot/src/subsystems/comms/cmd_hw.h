#pragma once

#include "ICmdHW.h"
#include "app_config.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

/**
 * @brief UART hardware implementation of ICmdHW.
 *        Uses Zephyr UART Async API for non-blocking RX/TX.
 *        Employs sys_ring_buf to buffer incoming bytes from DMA/interrupt.
 *
 * Public API: CmdHW_Init(), CmdHW_Transmit()
 */
class CmdHW : public ICmdHW {
public:
    CmdHW() = default;
    ~CmdHW() override = default;

    int  Init() override;
    int  Transmit(const uint8_t *data, size_t len) override;
    void RegisterRxCallback(RxByteCallback_t cb, void *ctx) override;

private:
    const struct device *m_uartDev  {nullptr};
    RxByteCallback_t     m_rxCb     {nullptr};
    void                *m_rxCtx    {nullptr};

    /* Static ring buffer for incoming data (interrupt-safe) */
    uint8_t  m_ringBufData[CMD_RING_BUF_SIZE] {};
    struct ring_buf m_ringBuf {};

    /* DMA/async RX double-buffer */
    uint8_t  m_rxBuf[CMD_UART_RX_BUF_SIZE]    {};

    bool m_initialized{false};

    /* Zephyr UART async callback (static trampoline) */
    static void UartAsyncCallback(const struct device *dev,
                                   struct uart_event *evt,
                                   void *userData);

    /** Process bytes stored in ring buffer and dispatch to m_rxCb */
    void DrainRingBuffer();
};
