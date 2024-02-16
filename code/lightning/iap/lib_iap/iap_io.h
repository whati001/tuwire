#ifndef IAP_IO_H
#define IAP_IO_H

#include "iap_defs.h"

/*
 * iap_core library physical layer abstraction
 * The idea is to use the code from the iap_core library independet
 * from the underlying PHY implementation.
 * Therefore, we will hide the properties behind a void* and remember
 * the type as iap_transport_type. The PHY only needs to implement the
 * function:
 *  - int iap_init_transport(iap_transport_t *trans, iap_transport_type mode, void *param);
 *  - int iap_transfer_out(iap_transport_t *trans, uint8_t res_needed);
 *  - int iap_transfer_in(iap_transport_t *trans, uint8_t res_needed);
 */

// define the size of the internal library buffers for reading and writing data
#define IAP_BUFFER_SIZE 512

/*
 * Define which type of transport is used
 * TODO: Currently only UART is really supported
 */
typedef enum
{
    IAP_UART,
    IAP_USB,
    IAP_BLE
} iap_transport_type;

/*
 * Define which errno types are supported by the iap transport
 */
typedef enum
{
    IAP_TRANS_ERRNO_OK,
    IAP_TRANS_ERRNO_TIMEOUT,
} iap_transport_errno_t;

/*
 * Configuration structs for each transport type
 */
typedef struct
{
    uint8_t tx_pin;
    uint8_t rx_pin;
    uint32_t baud_rate;
} iap_transport_uart_config_t;

typedef struct
{
    // TODO: add some usb config
} iap_transport_usb_config_t;

/*
 * iAP transport instance
 * Please consider to allocate this struct somewhere statically
 * Ensure that this struct is not allocated in a stackframe,
 * it's quite big and could consume to much memory
 */
typedef struct iap_transport_t
{
    iap_transport_type mode;
    uint8_t active;
    uint8_t idps_done;
    uint8_t authenticated;
    uint16_t transid;
    uint8_t iap_errno;
    uint16_t max_packet_size;
    uint32_t read_buf_len;
    uint8_t read_buf[IAP_BUFFER_SIZE];
    uint32_t write_buf_len;
    uint8_t write_buf[IAP_BUFFER_SIZE];
} iap_transport_t;

/*
 * Initiate a new transport instance based on the mode and param
 *
 * param:
 *  transport: iap_transport_t* -> NULL on error; otherwise the new created transport instance
 *  mode: iap_transport_type -> type which should be created
 *  config: void* -> transport specific instantiate configuration
 */
int iap_init_transport(iap_transport_t *trans, iap_transport_type mode, void *config);

/*
 * Execute a new outbound iAP request transfer for the provided transport
 * Because iAP always consists of a request and response, there is no need to separate it into read/write
 * Simple make a single transfer function, which contains a pointer where to store the response from the iDevice
 *
 * param
 *  transfer: iap_transport_t* -> transport to perform transfer on
 *  req: iap_command_t* -> iAP command send to iDevice
 *  res: iap_command* -> iAP command received from iDevice, on NULL reading will be skipped
 */
int iap_send_command(iap_transport_t *trans, iap_command_t *req_cmd, iap_command_t *res_cmd);
int iap_recv_command(iap_transport_t *trans, iap_command_t *req_cmd, iap_command_t *res_cmd);

/*
 * Execute a new outbound iAP request transfer for the provided transport
 * Because iAP always consists of a request and response, there is no need to separate it into read/write
 * Simple make a single transfer function, which contains a pointer where to store the response from the iDevice
 *
 * param
 *  transfer: iap_transport_t* -> transport to perform transfer on
 *  req: iap_msg_t* -> iAP message send to iDevice
 *  res: iap_msg_t* -> iAP message received from iDevice, on NULL reading will be skipped
 */
int iap_send_msg(iap_transport_t *trans, iap_msg_t *req_msg, iap_msg_t *res_msg);
int iap_recv_msg(iap_transport_t *trans, iap_msg_t *req_msg, iap_msg_t *res_msg);
/*
 * Execute a new outbound iAP message transfer for the provided transport
 * Because iAP always consists of a request and response, there is no need to separate it into read/write
 * Simple make a single transfer function, which will update only the member variables of the transport
 *
 * param
 *  transfer: iap_transport_t* -> transport to perform transfer on
 *  res_needed: uint8_t -> boolean if the response should be read or not
 */
int iap_transfer_out(iap_transport_t *trans, uint8_t res_needed);

/*
 * Execute a new inbound iAP message transfer for the provided transport
 * Because iAP always consists of a request and response, there is no need to separate it into read/write
 * Simple make a single transfer function, which will update only the member variables of the transport
 *
 * param
 *  transfer: iap_transport_t* -> transport to perform transfer on
 *  res_needed: uint8_t -> boolean if the response should be written or not
 */
int iap_transfer_in(iap_transport_t *trans, uint8_t res_needed);

#endif