#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "idbus.h"
#include "idbus_io.h"
#include "idbus_msg.h"
#include "idbus_util.h"

void idbus_print_tristar_info(idbus_tristar_info_t *info)
{
    if (NULL == info)
    {
        printf("Failed to print Tristar info because we have received an NULL pointer\n");
        return;
    }

    printf("Trister-Info{\n  seven_four: ");
    PRINT_HEX_ARRAY(info->seven_four_msg.data, LEN_SEVEN_FOUR);
    printf("\n  ios_version: ");
    PRINT_HEX_ARRAY(info->ios_version.data, LEN_SYSTEM_NOTIFICATION);
    printf("\n  model_number: ");
    PRINT_HEX_ARRAY(info->model_number.data, LEN_SYSTEM_NOTIFICATION);
    printf("\n}\n");
}

int idbus_init_tristar(idbus_tristar_t *tristar, idbus_tristar_info_t *info)
{
    idbus_debug("Start to initialize a new Tristar instance\n");
    if (NULL == tristar)
    {
        printf("Failed to initialize Tristar instance, because of NULL pointer as target\n");
        return IDBUS_ERR;
    }
    if (NULL == info)
    {
        printf("Failed to initialize Tristar instance, because of NULL pointer as source\n");
        return IDBUS_ERR;
    }

#if IDBUS_CORE_DEBUG == 1
    idbus_debug("Used config:\n");
    idbus_print_tristar_info(info);
#endif

    // prepare the 0x74 msg
    tristar->seven_four.header = 0x74;
    tristar->seven_four.data_len = LEN_SEVEN_FOUR_DATA;
    tristar->seven_four.data_ptr = tristar->seven_four_data;
    memcpy(tristar->seven_four_data, info->seven_four_msg.data, LEN_SEVEN_FOUR);
    idbus_compute_checksum(&tristar->seven_four);

    // prepare the 0x70 msg
    tristar->seven_zero.header = 0x70;
    tristar->seven_zero.data_len = LEN_SEVEN_ZERO_DATA;
    tristar->seven_zero.data_ptr = tristar->seven_zero_data;
    memcpy(tristar->seven_zero_data, info->charing_active.data, LEN_CHARGING_STATE);
    idbus_compute_checksum(&tristar->seven_zero);

    // prepare the 0x76 msg
    tristar->seven_six.header = 0x76;
    tristar->seven_six.data_len = LEN_SEVEN_SIX_DATA;
    tristar->seven_six.data_ptr = tristar->seven_six_data;
    idbus_compute_checksum(&tristar->seven_six);

    // prepare the 0x78 msg
    tristar->seven_eight.header = 0x78;
    tristar->seven_eight.data_len = LEN_SEVEN_EIGHT_DATA;
    tristar->seven_eight.data_ptr = tristar->seven_eight_data;
    idbus_compute_checksum(&tristar->seven_eight);

    // prepare the 0x7A msg
    tristar->seven_a.header = 0x7A;
    tristar->seven_a.data_len = LEN_SEVEN_A_DATA;
    tristar->seven_a.data_ptr = tristar->seven_a_data;
    idbus_compute_checksum(&tristar->seven_a);

    // prepare the 0x72 msg
    tristar->seven_two.header = 0x72;
    tristar->seven_two.data_len = LEN_SEVEN_TWO_DATA;
    tristar->seven_two.data_ptr = tristar->seven_two_data;
    idbus_compute_checksum(&tristar->seven_two);

    // prepare the first 0x84 msg
    tristar->eight_four.header = 0x84;
    tristar->eight_four.data_len = LEN_EIGHT_FOUR_DATA;
    tristar->eight_four.data_ptr = tristar->eight_four_data;
    tristar->eight_four_data[0] = 0x00;
    tristar->eight_four_data[1] = 0x00;
    tristar->eight_four_data[2] = LEN_SYSTEM_NOTIFICATION;
    memcpy(tristar->eight_four_data + 3, info->model_number.data, LEN_SYSTEM_NOTIFICATION);
    idbus_compute_checksum(&tristar->eight_four);

    // prepare the second 0x84 msg
    tristar->eight_four2.header = 0x84;
    tristar->eight_four2.data_len = LEN_EIGHT_FOUR_DATA;
    tristar->eight_four2.data_ptr = tristar->eight_four2_data;
    tristar->eight_four2_data[0] = 0x01;
    tristar->eight_four2_data[1] = 0x00;
    tristar->eight_four2_data[2] = LEN_SYSTEM_NOTIFICATION;
    memcpy(tristar->eight_four2_data + 3, info->ios_version.data, LEN_SYSTEM_NOTIFICATION);
    idbus_compute_checksum(&tristar->eight_four2);

    idbus_debug("Finished to initialize a new HiFive instance\n");
#if IDBUS_CORE_DEBUG == 1
    idbus_debug("Instance msgs:\n");
    idbus_print_tristar(tristar);
#endif
    return IDBUS_OK;
}

void idbus_print_tristar(idbus_tristar_t *tristar)
{
    if (NULL == tristar)
    {
        printf("Failed to print Tristar instance because we have received an NULL pointer\n");
        return;
    }

    printf("Tristar-Instance{\n");
    printf("  0x74-");
    idbus_print_msg(&tristar->seven_four);
    printf("  0x70-");
    idbus_print_msg(&tristar->seven_zero);
    printf("  0x76-");
    idbus_print_msg(&tristar->seven_six);
    printf("  0x78-");
    idbus_print_msg(&tristar->seven_eight);
    printf("  0x7A-");
    idbus_print_msg(&tristar->seven_a);
    printf("  0x72-");
    idbus_print_msg(&tristar->seven_two);
    printf("  0x84-");
    idbus_print_msg(&tristar->eight_four);
    printf("  0x84-");
    idbus_print_msg(&tristar->eight_four2);
    printf("}\n");
}

int idbus_map_tristar_response(idbus_msg_t **req, idbus_tristar_t *tristar, idbus_msg_t *res)
{
    int err = IDBUS_OK;
    // reply a request as Tristar based on the received request
    switch (res->header)
    {
    case 0x75:
        *req = &tristar->seven_zero;
        idbus_debug("Tristar has received 0x75 message\n");
        break;
    case 0x71:
        *req = &tristar->seven_six;
        idbus_debug("Tristar has received 0x71 message\n");
        break;
    case 0x77:
        *req = &tristar->seven_eight;
        idbus_debug("Tristar has received 0x77 message\n");
        break;
    case 0x79:
        *req = &tristar->seven_a;
        idbus_debug("Tristar has received 0x79 message\n");
        break;
    case 0x7B:
        *req = &tristar->seven_two;
        idbus_debug("Tristar has received 0x7B message\n");
        break;
    case 0x73:
        *req = &tristar->eight_four;
        idbus_debug("Tristar has received 0x73 message\n");

        *req = NULL;
        break;

    default:
        printf("Some unexpected response header value received:");
        idbus_print_msg(res);
        err = IDBUS_ERR;
        break;
    }

    return err;
}

int idbus_do_handshake_tristar(idbus_tristar_t *tristar, void *params)
{
    int err = IDBUS_OK;
    int fd = IDBUS_ERR;
    idbus_msg_t read_buffer = {0};
    idbus_msg_t *write_buffer = NULL;

    if (NULL == tristar)
    {
        printf("Failed to print Tristar instance because we have received an NULL pointer\n");
        return IDBUS_ERR;
    }

    printf("Start to perform IDBUS handshake as Tristar\n");

    // open the instance
    err = idbus_open_tristar(&fd, params);
    if (IDBUS_OK != err)
    {
        printf("Failed to open Tristar instance with error code: %d\n", fd);
        goto cleanup;
    }
    idbus_debug("Successfully opened Tristar instance with fd: %d\n", fd);

    // handle the handshake
    idbus_debug("Let's hope a HiFive is already connected, we will start requesting.\n");
    // point to the first request
    write_buffer = &tristar->seven_four;
    while (NULL != write_buffer)
    {

        // provide some time between each request
        sleep_ms(10);

        // write request to HiFive as Tristar
        err = idbus_write(fd, write_buffer);
        if (IDBUS_OK != err)
        {
            printf("Failed to write idbus message as Tristar with error code: %d\n", err);
            goto cleanup;
        }

        // receive a request from Tristar
        err = idbus_read(fd, &read_buffer);
        if (IDBUS_OK != err)
        {
            printf("Failed to read message from idbus as Tristar with following error code: %d\n", err);
            goto cleanup;
        }

        // map the response and update the write_buffer if needed
        err = idbus_map_tristar_response(&write_buffer, tristar, &read_buffer);
        if (IDBUS_OK != err)
        {
            // TODO: add some retry logic -> after some global retry count, we stop trying it
            idbus_debug("Some unexpected IDBUS message received as HiFive, let's wait for retry\n");
            continue;
        }
    }

    write_buffer = &tristar->eight_four;
    err = idbus_write(fd, write_buffer);
    if (IDBUS_OK != err)
    {
        printf("Failed to write idbus message as Tristar with error code: %d\n", err);
        goto cleanup;
    }
    sleep_ms(10);

    write_buffer = &tristar->eight_four2;
    err = idbus_write(fd, write_buffer);
    if (IDBUS_OK != err)
    {
        printf("Failed to write idbus message as Tristar with error code: %d\n", err);
        goto cleanup;
    }

    printf("Successfully performed IDBUS handshake as Tristar\n");

cleanup:
    if (IDBUS_ERR != fd)
    {
        idbus_debug("Close current Tristar instance\n");
        err = idbus_close(fd);
        if (IDBUS_ERR == err)
        {
            printf("Failed to close Tristar instance properly\n");
        }
    }
    return err;
}