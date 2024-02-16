#include <stdio.h>
#include <string.h>

#include "idbus.h"
#include "idbus_io.h"
#include "idbus_msg.h"
#include "idbus_util.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// TODO: find a better place than hardcoding this PIN
#define CHARGING_PIN 10

void idbus_print_hifive_info(idbus_hifive_info_t *info)
{
    if (NULL == info)
    {
        printf("Failed to print HiFive info because we have received an NULL pointer\n");
        return;
    }

    printf("HiFive-Info{\n  digital_id: ");
    PRINT_HEX_ARRAY(info->digital_id.data, LEN_DIGITIAL_ID);
    printf("\n  vendor_id: 0x%x", info->vendor_id);
    printf("\n  product_id: 0x%x", info->product_id);
    printf("\n  revision: 0x%x", info->revision);
    printf("\n  flags: 0x%x", info->flags);
    printf("\n  interface_serial_number: ");
    PRINT_HEX_ARRAY(info->interface_serial_number.data, LEN_INTERFACE_SERIAL_NUMBER);
    printf("\n  interface_module_number: ");
    PRINT_HEX_ARRAY(info->interface_module_number.data, LEN_INTERFACE_MODULE_NUMBER);
    printf("\n  accessory_serial_number: ");
    PRINT_HEX_ARRAY(info->accessory_serial_number.data, LEN_ACCESSORY_SERIAL_NUMBER);
    printf("\n  accessory_state: ");
    PRINT_HEX_ARRAY(info->accessory_state.data, LEN_ACCESSORY_STATE);
    printf("\n}\n");
}

int idbus_init_hifive(idbus_hifive_t *hifive, idbus_hifive_info_t *info)
{
    idbus_debug("Start to initialize a new HiFive instance\n");
    if (NULL == hifive)
    {
        printf("Failed to initialize HiFive instance, because of NULL pointer as target\n");
        return IDBUS_ERR;
    }
    if (NULL == info)
    {
        printf("Failed to initialize HiFive instance, because of NULL pointer as source\n");
        return IDBUS_ERR;
    }

#if IDBUS_CORE_DEBUG == 1
    idbus_debug("Used config:\n");
    idbus_print_hifive_info(info);
#endif

    // prepare the 0x75 msg
    hifive->seven_five.header = 0x75;
    hifive->seven_five.data_len = LEN_SEVEN_FIVE_DATA;
    hifive->seven_five.data_ptr = hifive->seven_five_data;
    memcpy(hifive->seven_five_data, info->digital_id.data, LEN_DIGITIAL_ID);
    idbus_compute_checksum(&hifive->seven_five);

    // prepare the 0x71 msg
    hifive->seven_one.header = 0x71;
    hifive->seven_one.data_len = LEN_SEVEN_ONE_DATA;
    hifive->seven_one.data_ptr = hifive->seven_one_data;
    idbus_compute_checksum(&hifive->seven_one);

    // prepare the 0x77 msg
    hifive->seven_seven.header = 0x77;
    hifive->seven_seven.data_len = LEN_SEVEN_SEVEN_DATA;
    hifive->seven_seven.data_ptr = hifive->seven_seven_data;
    hifive->seven_seven_data[0] = info->vendor_id;
    hifive->seven_seven_data[1] = info->product_id;
    hifive->seven_seven_data[2] = info->revision;
    hifive->seven_seven_data[3] = info->flags;
    memcpy(&hifive->seven_seven_data[4], info->interface_serial_number.data, LEN_INTERFACE_SERIAL_NUMBER);
    idbus_compute_checksum(&hifive->seven_seven);

    // prepare the 0x79 msg
    hifive->seven_nine.header = 0x79;
    hifive->seven_nine.data_len = LEN_SEVEN_NINE_DATA;
    hifive->seven_nine.data_ptr = hifive->seven_nine_data;
    memcpy(hifive->seven_nine_data, info->interface_module_number.data, LEN_INTERFACE_MODULE_NUMBER);
    idbus_compute_checksum(&hifive->seven_nine);

    // prepare the 0x7B msg
    hifive->seven_b.header = 0x7B;
    hifive->seven_b.data_len = LEN_SEVEN_B_DATA;
    hifive->seven_b.data_ptr = hifive->seven_b_data;
    memcpy(hifive->seven_b_data, info->accessory_serial_number.data, LEN_ACCESSORY_SERIAL_NUMBER);
    idbus_compute_checksum(&hifive->seven_b);

    // prepare teh 0x73 msg
    hifive->seven_three.header = 0x73;
    hifive->seven_three.data_len = LEN_SEVEN_THREE_DATA;
    hifive->seven_three.data_ptr = hifive->seven_three_data;
    memcpy(hifive->seven_three_data, info->accessory_state.data, LEN_ACCESSORY_STATE);
    idbus_compute_checksum(&hifive->seven_three);

    // prepare state values
    hifive->seven_one_count = 0;
    hifive->seven_eight_count = 0;

    idbus_debug("Finished to initialize a new HiFive instance\n");
#if IDBUS_CORE_DEBUG == 1
    idbus_debug("Instance msgs:\n");
    idbus_print_hifive(hifive);
#endif
    return IDBUS_OK;
}

void idbus_print_hifive(idbus_hifive_t *hifive)
{
    if (NULL == hifive)
    {
        printf("Failed to print HiFive instance because we have received an NULL pointer\n");
        return;
    }

    printf("HiFive-Instance{\n");
    printf("  0x75-");
    idbus_print_msg(&hifive->seven_five);
    printf("  0x71-");
    idbus_print_msg(&hifive->seven_one);
    printf("  0x77-");
    idbus_print_msg(&hifive->seven_seven);
    printf("  0x79-");
    idbus_print_msg(&hifive->seven_nine);
    printf("  0x7B-");
    idbus_print_msg(&hifive->seven_b);
    printf("  0x73-");
    idbus_print_msg(&hifive->seven_three);
    printf("}\n");
}

int idbus_map_hifive_response(idbus_msg_t **res, idbus_hifive_t *hifive, idbus_msg_t *req)
{
    int err = IDBUS_OK;
    // reply a response as HiFive
    switch (req->header)
    {
    case 0x74:
        *res = &hifive->seven_five;
        idbus_debug("HiFive has received 0x74 message\n");
        break;
    case 0x70:
        *res = &hifive->seven_one;
        hifive->seven_one_count++;
        idbus_debug("HiFive has received 0x70 message\n");
        break;
    case 0x76:
        *res = &hifive->seven_seven;
        idbus_debug("HiFive has received 0x76 message\n");
        break;
    case 0x78:
        *res = &hifive->seven_nine;
        idbus_debug("HiFive has received 0x78 message\n");
        break;
    case 0x7A:
        *res = &hifive->seven_b;
        idbus_debug("HiFive has received 0x7A message\n");
        break;
    case 0x72:
        *res = &hifive->seven_three;
        idbus_debug("HiFive has received 0x72 message\n");
        break;
    case 0x84:
        *res = NULL;
        hifive->seven_eight_count++;
        idbus_debug("HiFive has received 0x84 from Tristar, let's stop\n");
        break;

    default:
        printf("Some unexpected request header value received:");
        idbus_print_msg(req);
        err = IDBUS_ERR;
        break;
    }

    return err;
}

int idbus_do_handshake_hifive(idbus_hifive_t *hifive, void *params)
{
    int err = IDBUS_OK;
    int fd = IDBUS_ERR;
    idbus_msg_t read_buffer = {0};
    idbus_msg_t *write_buffer = NULL;

    if (NULL == hifive)
    {
        printf("Failed to print HiFive instance because we have received an NULL pointer\n");
        return IDBUS_ERR;
    }

    printf("Start to perform IDBUS handshake as HiFive\n");

    // init GPIO for power handshake
    hifive->seven_one_count = 0;
    hifive->seven_eight_count = 0;
    gpio_init(CHARGING_PIN);
    gpio_set_dir(CHARGING_PIN, GPIO_OUT);
    gpio_put(CHARGING_PIN, 1);

    // open the instance
    err = idbus_open_hifive(&fd, params);
    if (IDBUS_OK != err)
    {
        printf("Failed to open HiFive instance with error code: %d\n", fd);
        goto cleanup;
    }
    idbus_debug("Successfully opened HiFive instance with fd: %d\n", fd);

    // handle the handshake
    idbus_debug("Wait for Tristar to connect and send first IDBUS request\n");
    while (1)
    {
        // receive a request from Tristar
        err = idbus_read(fd, &read_buffer);
        if (IDBUS_OK != err)
        {
            printf("Failed to read message from idbus as HiFive with following error code: %d\n", err);
            goto cleanup;
        }
        // no need to sleep here, we have only a short time window for the response
        // the idbus_io library does not care about timing, the developer is responsible to
        // write the response in time. So please execute the write asap after the read
        // furthermore, please consider to compute the CRC checksum and everything else beforehand.
        err = idbus_map_hifive_response(&write_buffer, hifive, &read_buffer);
        if (IDBUS_OK != err)
        {
            // TODO: add some retry logic -> after some global retry count, we stop trying it
            idbus_debug("Some unexpected IDBUS message received as HiFive, let's wait for retry\n");
            continue;
        }
        // if we are done with the handshake
        if (hifive->seven_eight_count == 2)
        {
            printf("IDBUS handshake done, return from function\n");
            break;
        }
        // if we have received a request which does not require a response
        if (NULL == write_buffer)
        {
            continue;
        }

        err = idbus_write(fd, write_buffer);
        if (IDBUS_OK != err)
        {
            printf("Failed to write idbus message as HiFive with error code: %d\n", err);
            goto cleanup;
        }

        if (hifive->seven_one_count == 2)
        {
            printf("IDBUS perform charging handshake\n");
            sleep_ms(7);
            gpio_put(CHARGING_PIN, 0);
            sleep_ms(24);
            gpio_put(CHARGING_PIN, 1);
        }
    }

    printf("Successfully performed IDBUS handshake as HiFive\n");

cleanup:
    idbus_debug("Close current HiFive instance\n");
    err = idbus_close(fd);
    if (IDBUS_OK != err)
    {
        printf("Failed to close HiFive instance properly\n");
        err = IDBUS_ERR;
    }
    return err;
}