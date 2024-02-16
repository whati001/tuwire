#include <stdio.h>
#include <stddef.h>

#include "iap_core.h"
#include "iap_util.h"
#include "iap_io_usb.h"
#include "iap_io_uart.h"

int iap_init_transport(iap_transport_t *trans, iap_transport_type mode, void *config)
{
    if (trans->active)
    {
        printf("Transport already active, skip open request\n");
        return IAP_OK;
    }
    switch (mode)
    {
    case IAP_USB:
    {
        printf("Start to open a new iap_transport instance for USB\n");
        return _iap_init_transport_usb(trans, mode, config);
    }
    case IAP_UART:
    {
        debug("Start to open a new iap_transport instance for UART\n");
        return _iap_init_transport_uart(trans, mode, config);
    }
    default:
    {
        printf("Unsupported iap_transport mode requested\n");
        return IAP_ERR;
    }
    }
}

static int inline _iap_uart_read_first_byte(uint8_t *byte)
{
    return _iap_uart_read_byte(byte, IAP_UART_RES_TIMEOUT_MS);
}

static int inline _iap_uart_read_next_byte(uint8_t *byte)
{
    return _iap_uart_read_byte(byte, IAP_UART_BYTE_TIMEOUT_MS);
}

static int _iap_transfer_uart_read(uint8_t *buf, uint32_t *len)
{
    int err = IAP_OK;
    *len = 0;
    uint32_t msg_len = 0;

    // first read the header bytes
    err = _iap_uart_read_first_byte(buf++);
    if (err == IAP_ERR)
    {
        printf("No iAP response received at all\n");
        goto cleanup;
    }

    // read second header value
    err = _iap_uart_read_next_byte(buf++);
    if (err == IAP_ERR)
    {
        printf("Failed to read iAP header properly\n");
        goto cleanup;
    }
    // correct final read length by the header size
    *len = 2;

    // second, read the message length
    // iAP1 and iAP2 encodes the length differently
    // iAP1:
    //  - 1 byte: real value
    //  - 3 byte: 0x00 marker byte + 2 byte length (MSB, LSB)
    // iAP2:
    //  - 2 byte: real value (MSB, LSB)
#if IAP_VERSION == IAP1
    debug("Start to read the length of the iAP1 message");
    err = _iap_uart_read_next_byte(buf);
    if (err == IAP_ERR)
    {
        printf("Failed to read first byte from the iAP message length\n");
    }
    if (0x00 == *buf)
    {
        // TODO: verify if this code works, not tested yet
        debug("iAP1 message contains a 3-byte length value\n");
        buf++;
        err = _iap_uart_read_next_byte(buf);
        msg_len = (*buf++ << 8);
        buf++;
        err |= _iap_uart_read_next_byte(buf++);
        msg_len |= (*buf);
        buf++;
        if (err == IAP_ERR)
        {
            debug("Failed to read three bytes from the iAP message length\n");
            goto cleanup;
        }
        // correct final read length by the length variable size
        *len += 3;
    }
    else
    {
        debug("iAP1 message contains a 1-byte length value\n");
        msg_len = *buf;
        buf++;
        // correct final read length by the length variable size
        *len += 1;
    }
    debug("Read iAP1 message length: 0x%x\n", msg_len);
    // TODO: verify that we do not overflow the buffer
#else
// please here some code to read the iAP2 length
#error "iAP2 is not supported yet, please add it and create a PR"
#endif

    // third read the rest of the message payload
    // correct final read length by the payload size
    *len += msg_len;
    while (msg_len--)
    {
        err = _iap_uart_read_next_byte(buf++);
        if (err == IAP_ERR)
        {
            printf("Failed to read iAP payload properly\n");
            goto cleanup;
        }
    }

    // four read the checksum
    err = _iap_uart_read_next_byte(buf);
    if (err == IAP_ERR)
    {
        printf("No iAP checksum received a\n");
        goto cleanup;
    }
    // correct final read length by the checksum size
    *len += 1;
    debug("Read iAP message successfully\n");

cleanup:
    // void misuse of the read buffer by setting it's length to 0 on error
    if (err == IAP_ERR)
    {
        *len = 0;
    }
    return err;
}

int _iap_transfer_out_uart(iap_transport_t *trans, uint8_t res_needed)
{
    int err = IAP_OK;
    // send out write buffer
    err = _iap_uart_write_byte(trans->write_buf, trans->write_buf_len);
    CHECK_ERR(err, "Failed to write iAP message via UART\n")

    // read in read buffer
    if (!res_needed)
    {
        printf("No response reading is needed, skip reading\n");
        return err;
    }
    err = _iap_transfer_uart_read(trans->read_buf, &trans->read_buf_len);
    CHECK_ERR(err, "Failed to read iAP message via UART\n")

    return err;
}

int _iap_transfer_in_uart(iap_transport_t *trans, uint8_t res_needed)
{
    int err = IAP_OK;
    // read in data
    err = _iap_transfer_uart_read(trans->read_buf, &trans->read_buf_len);
    CHECK_ERR(err, "Failed to read iAP message via UART\n");

    if (!res_needed)
    {
        printf("No response sending is needed, skip writing\n");
        return err;
    }
    err = _iap_uart_write_byte(trans->write_buf, trans->write_buf_len);
    CHECK_ERR(err, "Failed to write iAP message via UART\n")

    return err;
}

int iap_transfer_out(iap_transport_t *trans, uint8_t res_needed)
{
    switch (trans->mode)
    {
    case IAP_USB:
    {
        printf("Start to write some data via iap_transport instance for USB\n");
        return _iap_transfer_out_usb(trans, res_needed);
    }
    case IAP_UART:
    {
        debug("Start to transfer out some data via iap_transport instance for UART\n");
        return _iap_transfer_out_uart(trans, res_needed);
    }
    default:
    {
        printf("Unsupported iap_transport mode requested for OUT transfer\n");
        return IAP_ERR;
    }
    }
}

int iap_transfer_in(iap_transport_t *trans, uint8_t res_needed)
{
    switch (trans->mode)
    {
    case IAP_USB:
    {
        printf("Start to read in some data via iap_transport instance for USB\n");
        return _iap_transfer_in_usb(trans, res_needed);
    }
    case IAP_UART:
    {
        printf("Start to read in some data via iap_transport instance for UART\n");
        return _iap_transfer_in_uart(trans, res_needed);
    }
    default:
    {
        printf("Unsupported iap_transport mode requested for IN transfer\n");
        return IAP_ERR;
    }
    }
}
