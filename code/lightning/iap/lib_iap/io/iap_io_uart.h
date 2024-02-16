#ifndef IAP_IO_UART_H
#define IAP_IO_UART_H

#include "iap_io.h"

// define the read timeout
#define IAP_UART_RES_TIMEOUT_MS 5000
#define IAP_UART_BYTE_TIMEOUT_MS 100

/*
 * UART is exchanged byte by byte
 * Therefore, the individual hardware abstraction layer
 * only needs to provide simple read write functions
 * for a single byte
 *
 * TODO: add some logic to handle more iap_transport_t
 * instances via multiple UART instances
 * This requires to pass the iap_transport_t struct
 * to the _iap_uart_write_byte and _iap_uart_read_byte function
 */
int _iap_init_transport_uart(iap_transport_t *trans, iap_transport_type mode, void *param);
int _iap_uart_write_byte(uint8_t *buf, uint32_t len);
int _iap_uart_read_byte(uint8_t *byte, uint32_t timeout);

#endif