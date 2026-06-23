#pragma once

/*
 * ICmdHW.h - Pure abstract interface for command hardware (UART/Bluetooth/WiFi)
 * cmd_receiver and cmd_transfer depend ONLY on this interface, never on cmd_hw.h.
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Pure abstract interface between cmd_receiver/cmd_transfer and hardware.
 *        Decouples communication logic from the physical transport (UART, BT, WiFi).
 */
class ICmdHW {
public:
    /** Callback invoked for each byte received from the master device */
    using RxByteCallback_t = void (*)(uint8_t byte, void *ctx);

    virtual ~ICmdHW() = default;

    /**
     * @brief Initialize the hardware peripheral (UART, DMA, ring buffer).
     * @return 0 on success, negative errno on failure.
     */
    virtual int Init() = 0;

    /**
     * @brief Transmit a data buffer to the master device.
     *        Uses async/interrupt API - non-blocking where possible.
     * @param data Pointer to data buffer.
     * @param len  Number of bytes to transmit.
     * @return 0 on success, negative errno on failure.
     */
    virtual int Transmit(const uint8_t *data, size_t len) = 0;

    /**
     * @brief Register a callback to receive bytes as they arrive.
     *        Callback is invoked from interrupt context - keep minimal.
     * @param cb   Byte-received callback.
     * @param ctx  User context passed to cb.
     */
    virtual void RegisterRxCallback(RxByteCallback_t cb, void *ctx) = 0;
};
