#include "cmd_hw.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cmd_hw, LOG_LEVEL_DBG);

#define CMD_UART_NODE DT_NODELABEL(usart2)

int CmdHW::Init()
{
    m_uartDev = DEVICE_DT_GET(CMD_UART_NODE);
    if (!device_is_ready(m_uartDev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

    /* Initialize ring buffer for RX buffering */
    ring_buf_init(&m_ringBuf, sizeof(m_ringBufData), m_ringBufData);

    /* Register async callback */
    int ret = uart_callback_set(m_uartDev, UartAsyncCallback, this);
    if (ret < 0) {
        LOG_ERR("UART callback set failed: %d", ret);
        return ret;
    }

    /* Enable async RX with double-buffer and timeout */
    ret = uart_rx_enable(m_uartDev, m_rxBuf, sizeof(m_rxBuf), CMD_UART_TIMEOUT_US);
    if (ret < 0) {
        LOG_ERR("UART RX enable failed: %d", ret);
        return ret;
    }

    m_initialized = true;
    LOG_INF("CmdHW initialized: USART2 @ 115200 baud");
    return 0;
}

int CmdHW::Transmit(const uint8_t *data, size_t len)
{
    if (!m_initialized || data == nullptr || len == 0U) {
        return -EINVAL;
    }

    /* uart_tx is async - buffer must remain valid until TX_DONE callback.
     * For simplicity, we use K_FOREVER timeout (blocking until TX accepted). */
    int ret = uart_tx(m_uartDev, data, len, SYS_FOREVER_US);
    if (ret < 0) {
        LOG_ERR("UART TX failed: %d", ret);
        return ret;
    }
    return 0;
}

void CmdHW::RegisterRxCallback(RxByteCallback_t cb, void *ctx)
{
    m_rxCb  = cb;
    m_rxCtx = ctx;
}

void CmdHW::UartAsyncCallback(const struct device *dev,
                                struct uart_event *evt,
                                void *userData)
{
    ARG_UNUSED(dev);
    CmdHW *self = static_cast<CmdHW *>(userData);

    switch (evt->type) {
    case UART_RX_RDY:
        /* Data received - push to ring buffer from ISR context */
        {
            const uint8_t *buf = evt->data.rx.buf + evt->data.rx.offset;
            uint32_t       len = evt->data.rx.len;
            ring_buf_put(&self->m_ringBuf, buf, len);
            self->DrainRingBuffer();
        }
        break;

    case UART_RX_BUF_REQUEST:
        /* Provide a new buffer - reuse same buffer (single-buffer mode) */
        uart_rx_buf_rsp(dev, self->m_rxBuf, sizeof(self->m_rxBuf));
        break;

    case UART_RX_DISABLED:
        /* Re-enable RX if it was disabled (timeout or buffer overflow) */
        uart_rx_enable(dev, self->m_rxBuf, sizeof(self->m_rxBuf), CMD_UART_TIMEOUT_US);
        break;

    case UART_TX_DONE:
        LOG_DBG("CmdHW: TX done");
        break;

    case UART_TX_ABORTED:
        LOG_WRN("CmdHW: TX aborted");
        break;

    default:
        break;
    }
}

void CmdHW::DrainRingBuffer()
{
    if (m_rxCb == nullptr) {
        return;
    }

    uint8_t byte;
    while (ring_buf_get(&m_ringBuf, &byte, 1U) == 1U) {
        m_rxCb(byte, m_rxCtx);
    }
}
