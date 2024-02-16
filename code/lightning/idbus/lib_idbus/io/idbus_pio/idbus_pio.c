/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include "idbus_pio.h"

#include "idbus_pio_rx.pio.h"
#include "idbus_pio_tx.pio.h"
#include "idbus_pio_wake.pio.h"

int __idbus_pin_is_used(uint8_t pin)
{
    int ret = IDBUS_PIO_OK;
    for (uint8_t idx = 0; idx < IDBUS_PIO_INSTANCE_COUNT; idx++)
    {
        idbus_pio_t *inst = &idbus_pio_inst[idx];
        idbus_debug("Check if pin is used against inst[%d]{active:%d, pin:%d}\n", idx, inst->active, inst->pin);
        if (IDBUS_PIO_OK == __idbus_instance_is_active(inst))
        {
            if (pin == inst->pin)
            {
                ret = IDBUS_PIO_ERROR;
                break;
            }
        }
    }

    return ret;
}

int __idbus_claim_free_instance(uint8_t *fd)
{
    int ret = IDBUS_PIO_NO_FREE_INSTANCE;
    for (uint8_t idx = 0; idx < IDBUS_PIO_INSTANCE_COUNT; idx++)
    {
        idbus_pio_t *inst = &idbus_pio_inst[idx];
        if (IDBUS_PIO_OK != __idbus_instance_is_active(inst))
        {
            ret = IDBUS_PIO_OK;
            *fd = idx;
            break;
        }
    }

    return ret;
}

void __idbus_clear_instance(uint8_t idx)
{
    idbus_pio_t *inst = &idbus_pio_inst[idx];
    inst->active = 1;
    inst->pio = IDBUS_PIO_PIO_INSTANCE;
    inst->sm = 0;
    inst->offset = 0;
    inst->clkdiv = IDBUS_PIO_CLKDIV;
    inst->chip = 0;
    inst->pin = 0;
    inst->buf_len = 0;
    memset(inst->buf_data, 0, IDBUS_PIO_MSG_BUFFER_SIZE);
}

int open_idbus_pio(int *fd, enum CHIP chip, uint8_t pin)
{
    int ret = IDBUS_PIO_OK;
    uint8_t id = 0;
    idbus_pio_t *inst = NULL;
    *fd = IDBUS_PIO_ERROR;

    idbus_debug("Requested to open a new idbus_pio instance for pin: %d\n", pin);
    // perform some params checks
    ret = __idbus_pin_is_used(pin);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to open idbus_pio instance, the pin is already used\n");
        ret = IDBUS_PIO_NO_UNIQUE_PIN;
        goto cleanup;
    }

    ret = __idbus_claim_free_instance(&id);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to open idbus_pio instance, the pin is already used\n");
        ret = IDBUS_PIO_NO_FREE_INSTANCE;
        goto cleanup;
    }

    ret = __idbus_is_valid_chip(chip);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to open idbus_pio instance, the chip type is not supported\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    // acquire a free idbus_pio instance
    __idbus_clear_instance(id);
    inst = &idbus_pio_inst[id];
    inst->pin = pin;
    inst->chip = chip;

    // return a valid file descriptor (idx) pointing into idbus_pio_inst
    *fd = id;
    idbus_debug("Acquired a new idbus_pio instance for pin: %d\n", inst->pin);

cleanup:
    return ret;
}

void __idbus_buffer_msg(uint8_t id, idbus_msg_t *msg)
{
    // copy over the msg to make the lib more stable
    idbus_pio_t *inst = &idbus_pio_inst[id];
    uint8_t *data_ptr = inst->buf_data;

    uint8_t max_data_len = msg->data_len;
    if (IDBUS_PIO_DATA_BUFFER_SIZE < msg->data_len)
    {
        printf("Warning, the idbus message data section exceeds the maximum defined buffer size for sending, please increase IDBUS_PIO_DATA_BUFFER_SIZE if needed\n");
        max_data_len = IDBUS_PIO_DATA_BUFFER_SIZE;
    }

    *data_ptr++ = msg->header;
    memcpy(data_ptr, msg->data_ptr, max_data_len);
    data_ptr += max_data_len;
    *data_ptr = msg->crc;

    inst->buf_len = max_data_len + IDBUS_PIO_META_BUFFER_SIZE;
}

int __idbus_unbuffer_msg(uint8_t id, idbus_msg_t *msg)
{
    // copy out the msg to make the lib more stable
    idbus_pio_t *inst = &idbus_pio_inst[id];

    if (0 == inst->buf_len)
    {
        printf("No data received, read run into timeout\n");
        return IDBUS_PIO_TIMEOUT;
    }
    if (2 > inst->buf_len)
    {
        printf("Not enough data received, less than two bytes but header and crc is mandatory\n");
        return IDBUS_PIO_ERROR;
    }
    msg->header = inst->buf_data[0];
    msg->data_len = inst->buf_len - 2;
    memcpy(msg->data_ptr, (inst->buf_data + 1), msg->data_len);
    msg->crc = inst->buf_data[(msg->data_len + 1)];

    return IDBUS_PIO_OK;
}

void __idbus_clear_buffer_msg(uint8_t id)
{
    // copy over the msg to make the lib more stable
    idbus_pio_t *inst = &idbus_pio_inst[id];
    memset(inst->buf_data, 0, inst->buf_len);
    inst->buf_len = 0;
}

void __idbus_reset_pio(uint8_t id)
{
    idbus_pio_t *inst = &idbus_pio_inst[id];
    pio_sm_set_enabled(inst->pio, inst->sm, false);
    pio_sm_unclaim(inst->pio, inst->sm);
    pio_sm_clear_fifos(inst->pio, inst->sm);
    pio_clear_instruction_memory(inst->pio);
}

int __idbus_active_rx(uint8_t id)
{
    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = &idbus_pio_inst[id];
    uint offset, sm;
    void (*setup)(PIO pio, uint sm, uint offset, uint pin, float clkdiv);

    __idbus_reset_pio(id);

    if (TRISTAR == inst->chip)
    {
        goto cleanup;
    }

    offset = pio_add_program(inst->pio, &idbus_rx_program);
    sm = pio_claim_unused_sm(inst->pio, false);
    if (IDBUS_PIO_ERROR == sm)
    {
        printf("IDBUS read failed, because no free sm was found, ret value: %d\n", sm);
        ret = IDBUS_PIO_NO_FREE_SM;
        goto cleanup;
    }

    inst->sm = sm;
    inst->offset = offset;
    idbus_rx_program_init(inst);

cleanup:
    if (IDBUS_PIO_OK != ret)
    {
        __idbus_reset_pio(id);
    }
    return ret;
}

int __idbus_active_tx(uint8_t id)
{
    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = &idbus_pio_inst[id];
    uint offset, sm;

    __idbus_reset_pio(id);

    offset = pio_add_program(inst->pio, &idbus_tx_program);
    sm = pio_claim_unused_sm(inst->pio, false);
    if (IDBUS_PIO_ERROR == sm)
    {
        printf("IDBUS write failed, because no free sm was found.\n");
        ret = IDBUS_PIO_NO_FREE_SM;
        goto cleanup;
    }

    inst->sm = sm;
    inst->offset = offset;
    idbus_tx_program_init(inst);

cleanup:
    if (IDBUS_PIO_OK != ret)
    {
        __idbus_reset_pio(id);
    }
    return ret;
}

int __raw_write_idbus_msg(uint8_t id)
{
    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = &idbus_pio_inst[id];
    uint8_t is_tristar = (TRISTAR == inst->chip);

    // register how many bytes the message consists of
    pio_sm_put_blocking(inst->pio, inst->sm, (inst->buf_len - 1));

    // register if a BREAK is needed
    pio_sm_put_blocking(inst->pio, inst->sm, is_tristar);

    for (uint8_t idx = 0; idx < inst->buf_len; idx++)
    {
        pio_sm_put_blocking(inst->pio, inst->sm, inst->buf_data[idx]);
    }

    // register if a BREAK is needed
    pio_sm_put_blocking(inst->pio, inst->sm, is_tristar);

    // waiting for PIO code to finish sending
    if (1 != pio_sm_get_blocking(inst->pio, inst->sm))
    {
        printf("Writing to IDBUS has returned wrong checksum\n");
        ret = IDBUS_PIO_ERROR;
    }

#if IDBUS_CORE_DEBUG == 1

    idbus_debug("Written following message bytes to bus as %s: ", is_tristar ? "TRISTAR" : "HIFIVE");
    __idbus_print_buffer(id);
#endif

    return ret;
}

int __raw_read_idbus_msg(uint8_t id, uint32_t timeout)
{

    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = &idbus_pio_inst[id];
    uint8_t is_tristar = (TRISTAR == inst->chip);

    uint8_t *data_ptr = inst->buf_data;
    uint8_t data_len = 0;
    uint32_t raw, value;
    uint8_t data_received = 0;
    uint32_t interrupt_mask = 0;

    uint32_t timeout_counter = 0;
    uint8_t timeout_inactive = (timeout == ((uint32_t)-1));
    uint8_t timeout_received = 1;
    uint32_t timeout_current = timeout / IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_POLL_US;

    while ((timeout_counter < timeout_current) || (timeout_inactive))
    {
        // maintain the timeout counter
        if (pio_sm_is_rx_fifo_empty(inst->pio, inst->sm))
        {
            sleep_us(IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_POLL_US);
            timeout_counter++;

            // break out of the loop, because we do not expect any more data
            if (data_received &&
                timeout_counter == IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_COUNTER)
            {
                timeout_received = 0;
                break;
            }
            continue;
        }

        // read the value and convert it into the proper format
        raw = pio_sm_get_blocking(inst->pio, inst->sm);
        if (0 == data_received && raw == IDBUS_PIO_BREAK_MSG)
        {
            continue;
        }
        if (1 == data_received && raw == IDBUS_PIO_BREAK_MSG)
        {
            timeout_received = 0;
            break;
        }
        timeout_counter = 0;
        data_received = 1;

        value = __idbus_reverse_byte(raw);
        *data_ptr++ = value;
        data_len++;

        // if the maximum amount of bytes have been read
        if (data_len == IDBUS_PIO_MSG_BUFFER_SIZE)
        {
            printf("Received maximum amount of bytes, please increase IDBUS_PIO_MSG_BUFFER_SIZE if needed\n");
            break;
        }
    }

    inst->buf_len = data_len;
#if IDBUS_CORE_DEBUG == 1
    idbus_debug("Read following message bytes from bus as %s: ", is_tristar ? "TRISTAR" : "HIFIVE");
    __idbus_print_buffer(id);
    // idbus_debug("TimeoutCoutner: %d -> byteTimeout: %d\n", timeout_counter, IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_COUNTER);
#endif
    if (timeout_received)
    {
        idbus_debug("Read run into message timeout\n");
        ret = IDBUS_PIO_TIMEOUT;
    }

    return ret;
}

int write_idbus_pio(int fd, idbus_msg_t *msg)
{
    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = NULL;
    uint8_t id = (uint8_t)fd;

    // perform some args validation
    ret = __idbus_is_valid_fd(fd);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to write idbus_pio message, the passed fd is not valid\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    ret = __idbus_is_valid_idbus_msg(msg);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to write idbus_pio message, the passed idbus msg is not valid\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    // retrieve the instance from the fd
    inst = &idbus_pio_inst[id];

    // buffer the msg for safety reason
    __idbus_clear_buffer_msg(id);
    __idbus_buffer_msg(id, msg);

    // activate tx mode if not already activated
    ret = __idbus_active_tx(id);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to write idbus_pio message, not able to active TX mode\n");
        goto cleanup;
    }

    // send out msg
    ret = __raw_write_idbus_msg(id);

    // if TRISTAR, we will read back the value to hold the time constrains
    if (TRISTAR == inst->chip)
    {
        __raw_read_idbus_msg(id, IDBUS_PIO_RX_NEXT_MSG_TIMEOUT_COUNTER);
    }

cleanup:
    return ret;
}

// https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte
// https://github.com/stacksmashing/tamarin-firmware/blob/51f7be33fa5a9d3cc75cd51abde5aa26637786b2/main.c#L65
uint8_t __idbus_reverse_byte(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

int read_idbus_pio(int fd, idbus_msg_t *msg)
{
    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = NULL;
    uint8_t id = (uint8_t)fd;

    // perform some args validation
    ret = __idbus_is_valid_fd(fd);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to write idbus_pio message, the passed fd is not valid\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    ret = __idbus_is_valid_idbus_msg(msg);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to write idbus_pio message, the passed idbus msg is not valid\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    // retrieve the instance from the fd
    inst = &idbus_pio_inst[id];

    // activate rx mode if not already activated
    ret = __idbus_active_rx(id);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to read idbus_pio message, not able to active RX mode\n");
        goto cleanup;
    }

    // TRISTAR has already read the values during the request write
    if (TRISTAR != inst->chip)
    {
        // clear the msg buffer and make room for reading values
        __idbus_clear_buffer_msg(id);

        // read in msg
        ret = __raw_read_idbus_msg(id, ((uint32_t)-1));
        if (IDBUS_PIO_OK != ret)
        {
            printf("Failed to read idbus_pio message, not all parts read properly\n");
            goto cleanup;
        }
    }

    // copy back the message to the user
    ret = __idbus_unbuffer_msg(id, msg);

cleanup:
    return ret;
}

void __idbus_print_buffer(uint8_t id)
{
    if (IDBUS_PIO_OK != __idbus_is_valid_fd(id))
    {
        printf("Received invalid fd to print message from\n");
        return;
    }

    idbus_pio_t *inst = &idbus_pio_inst[id];
    idbus_debug("IDBUS-Buffer{len: %d, data:[", inst->buf_len);
    for (uint8_t idx = 0; idx < inst->buf_len; idx++)
    {
        if (idx > 0)
        {
            idbus_debug(",");
        }
        idbus_debug("0x%x", inst->buf_data[idx]);
    }
    idbus_debug("]}\n");
}

int idbus_reset_pio(int fd)
{
    int ret = IDBUS_PIO_OK;
    uint8_t id = fd;
    uint offset = 0;
    int sm = 0;
    idbus_pio_t *inst = NULL;

    // perform some args validation
    ret = __idbus_is_valid_fd(fd);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to close idbus_pio message, the passed fd is not valid\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    // retrieve the instance from the fd
    inst = &idbus_pio_inst[fd];

    // check if HIFIVE
    if (HIFIVE != inst->chip)
    {
        printf("Reset idbus_pio instance is only supported for HIFIVE chip emulation\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    // clear the instance
    __idbus_clear_buffer_msg(fd);

    // send out WAKE
    __idbus_reset_pio(id);
    offset = pio_add_program(inst->pio, &idbus_wake_program);
    sm = pio_claim_unused_sm(inst->pio, false);
    if (IDBUS_PIO_ERROR == sm)
    {
        printf("IDBUS read failed, because no free sm was found, ret value: %d\n", sm);
        ret = IDBUS_PIO_NO_FREE_SM;
        goto cleanup;
    }

    inst->sm = sm;
    inst->offset = offset;
    idbus_wake_program_init(inst);

    int v = pio_sm_get_blocking(inst->pio, inst->sm);
    printf("Received value: %d\n", v);

cleanup:
    return ret;
}

int close_idbus_pio(int fd)
{
    int ret = IDBUS_PIO_OK;
    idbus_pio_t *inst = NULL;

    // perform some args validation
    ret = __idbus_is_valid_fd(fd);
    if (IDBUS_PIO_OK != ret)
    {
        printf("Failed to close idbus_pio message, the passed fd is not valid\n");
        ret = IDBUS_PIO_INVALID_PARAMS;
        goto cleanup;
    }

    // retrieve the instance from the fd
    inst = &idbus_pio_inst[fd];

    // clear the instance
    __idbus_clear_buffer_msg(fd);
    inst->active = 0;

    // clear pio instance
    __idbus_reset_pio(fd);

cleanup:
    return ret;
}