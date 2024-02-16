#include <stdio.h>

#include "crc.h"
#include "idbus.h"
#include "idbus_util.h"

void idbus_compute_checksum(idbus_msg_t *msg)
{
    crc part = crc_checksum_generic(&msg->header, 1, POLYNOMIAL, INITIAL_REMAINDER, TRUE, FALSE, FINAL_XOR_VALUE);
    msg->crc = crc_checksum_generic(msg->data_ptr, msg->data_len, POLYNOMIAL, part, TRUE, TRUE, FINAL_XOR_VALUE);
}

void idbus_print_msg(idbus_msg_t *msg)
{
    if (IDBUS_OK != __idbus_is_valid_idbus_msg(msg))
    {
        printf("Received invalid idbus message to print\n");
        return;
    }

    printf("IDBUS-Msg{header: 0x%x, len: %d, data:[", msg->header, msg->data_len);
    for (uint8_t idx = 0; idx < msg->data_len; idx++)
    {
        if (idx > 0)
        {
            printf(",");
        }
        printf("0x%x", msg->data_ptr[idx]);
    }
    printf("], crc: 0x%x}\n", msg->crc);
}
