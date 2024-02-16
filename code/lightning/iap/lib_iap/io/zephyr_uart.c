
#include "iap_io_uart.h"
#include "iap_core.h"
#include "iap_util.h"

int _iap_init_transport_uart(iap_transport_t *trans, iap_transport_type mode, void *param)
{
    return IAP_ERR;
}

int _iap_transfer_out_uart(iap_transport_t *trans, uint8_t res_needed)
{
    return IAP_ERR;
}

int _iap_transfer_in_uart(iap_transport_t *trans, uint8_t res_needed)
{
    return IAP_ERR;
}

/*
 * This is working iAP UART implementation for the Zephyr RTOS
 * But it was used in a dedicated Zephyr APP instead of in this
 * library, therefore, the code is commended out. 
 * Despite, it should work perfectly...
*/
/*
#include <stdio.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include "iap_io_uart.h"
#include "iap_core.h"
#include "iap_io.h"

// define the log level for the nero module
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(iap_io_uart);

#define UART_DEVICE_NODE DT_NODELABEL(arduino_serial)
// #define UART_DEVICE_NODE DT_NODELABEL(uart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

K_FIFO_DEFINE(recv_data);
// TODO: fix the buffer size -> this should be properly more
K_MSGQ_DEFINE(uart_msgq, 1, 50, 1);

void _iap_uart_rx_irq(const struct device *dev, void *user_data)
{
    uint8_t c = 0;

    if (!uart_irq_update(dev))
    {
        return;
    }

    while (uart_irq_rx_ready(dev))
    {
        uart_fifo_read(dev, &c, 1);
        k_msgq_put(&uart_msgq, &c, K_NO_WAIT);
    }
}

int _iap_uart_write_byte(uint8_t *buf, uint32_t len)
{
    debug("iap_core: Start to send out iAP message via UART\n");
    for (uint8_t idx = 0; idx < len; idx++)
    {
        uart_poll_out(uart_dev, buf[idx]);
    }
    debug("iap_core: Successfully send out iAP message via UART\n");
    return IAP_OK;
}

int _iap_uart_read_byte(uint8_t *byte, uint32_t timeout)
{
    if (0 == k_msgq_get(&uart_msgq, byte, K_MSEC(timeout)))
    {
        return IAP_OK;
    }
    printf("iap_core: iAP read run into timeout: %dms\n", timeout);
    return IAP_ERR;
}

int _iap_init_transport_uart(iap_transport_t *trans, iap_transport_type mode, void *param)
{
    int err = IAP_OK;
    if (IAP_UART != mode)
    {
        printf("iap_core: Please call the function _iap_init_transport_uart only with IAP_UART mode\n");
        return IAP_ERR;
    }
    trans->mode = mode,
    trans->active = 1;
    trans->idps_done = 0;
    trans->authenticated = 0;
    trans->transid = 0;
    trans->max_packet_size = 0;

    // current zephyr impl does not support conf
    err = !device_is_ready(uart_dev);
    CHECK_ERR(err, "Error: UART Device is not ready for iAP transport client.\n");
    err = uart_irq_callback_user_data_set(uart_dev, _iap_uart_rx_irq, trans);
    CHECK_ERR(err, "Failed to setup interrupt processing for iAP UART transport client\n");
    uart_irq_rx_enable(uart_dev);

    // we do not need all this stuff here, so let's keep everything else untouched
    return IAP_OK;
}
*/