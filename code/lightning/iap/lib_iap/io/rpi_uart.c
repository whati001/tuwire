#include <stdio.h>
#include <stddef.h>
#include <pico/stdlib.h>
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "iap_io_uart.h"
#include "iap_core.h"
#include "iap_util.h"

/*
 * Define the UART used for the USB-OTG handshake
 * Please ensure, that this uart is not used somewhere else
 * and connected properly to the Apple device
 * TODO: currently only one simultaneously UART instance is supported
 *       because we always use the uart0 instances
 */
#define IAP_DEF_UART_ID uart0
#define IAP_DEF_BAUD_RATE 19200
#define IAP_DEF_UART_TX_PIN 0
#define IAP_DEF_UART_RX_PIN 1

uart_inst_t *inst = IAP_DEF_UART_ID;

int _iap_uart_write_byte(uint8_t *buf, uint32_t len)
{
    for (uint32_t idx = 0; idx < len; idx++)
    {
        uart_putc_raw(inst, buf[idx]);
    }

    return IAP_OK;
}

int _iap_uart_read_byte(uint8_t *byte, uint32_t timeout)
{

    uint32_t timeout_ctr = 0;
    do
    {
        // if there is a byte waiting read and reset timeout counter
        if (uart_is_readable(inst))
        {
            *byte = (uint8_t)uart_get_hw(inst)->dr;
            // printf("Byte: 0x%x\n",*byte);
            return IAP_OK;
        }

        // increase the timeout counter
        sleep_ms(1);
        timeout_ctr++;
    } while (timeout > timeout_ctr);

    printf("iAP read run into timeout: %dms\n", timeout);
    return IAP_ERR;
}

int _iap_init_transport_uart(iap_transport_t *trans, iap_transport_type mode, void *config)
{
    int err = IAP_OK;
    uint8_t tx_pin = IAP_DEF_UART_TX_PIN;
    uint8_t rx_pin = IAP_DEF_UART_RX_PIN;
    uint32_t baud_rate = IAP_DEF_BAUD_RATE;

    if (NULL != config)
    {
        iap_transport_uart_config_t *config = (iap_transport_uart_config_t *)config;
        tx_pin = config->tx_pin;
        rx_pin = config->rx_pin;
        baud_rate = config->baud_rate;
    }

    // initialize the UART instance
    printf("Initialize a new UART instance:{\n - tx_pin: %d\n - rx_pin: %d\n - baud: %d\n}\n", tx_pin, rx_pin, baud_rate);
    uart_init(inst, baud_rate);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);

    trans->active = 1;
    trans->idps_done = 0;
    trans->authenticated = 0;
    trans->transid = 0;
    trans->max_packet_size = 0;

    return IAP_OK;
}
