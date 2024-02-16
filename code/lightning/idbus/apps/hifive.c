/**********************************************************************
 *
 * Filename:    hifive.c
 *
 * Description: Simple example, how to emulate the Apple HIFIVE chip
 *
 **********************************************************************/

#include <stdio.h>
#include <pico/stdlib.h>
#include "idbus.h"
#include "util.h"

/*
 * Define the PIN number used for the IDBUS communication
 * Please ensure, that the breakout board ID pin is connected to this
 * GPIO pin -> GPIO3 in this case
 */
#define IDBUS_PIN 3

static int fd = 0, err = 0;
static idbus_hifive_t hifive = {0};
static idbus_hifive_info_t hifive_info = {0};

/*
 * load the HiFive information
 * This example uses hardcoded values, however this could be loaded from disk, network, etc.
 */
int load_hifive_info(idbus_hifive_info_t *hifive_info)
{
    // 0x76 response message
    hifive_info->digital_id = (digital_id_t){0x10, 0x0C, 0x00, 0x00, 0x00, 0x00};

    // 0x77 response message
    hifive_info->vendor_id = 0x02;
    hifive_info->product_id = 0x01;
    hifive_info->revision = 0x02;
    hifive_info->flags = 0x80;
    hifive_info->interface_serial_number = (interface_serial_number_t){0x60, 0x01, 0x26, 0xDD, 0xAA, 0x6F};

    // 0x79 response message
    hifive_info->interface_module_number = (interface_module_number_t){0x44, 0x59, 0x47, 0x37, 0x32, 0x38, 0x35, 0x55, 0x50, 0x39, 0x56, 0x46, 0x4A, 0x59, 0x48, 0x41, 0x59, 0x00, 0x65, 0x88};

    // 0x7B response message
    hifive_info->accessory_serial_number = (accessory_serial_number_t){0x46, 0x43, 0x39, 0x37, 0x33, 0x31, 0x35, 0x32, 0x39, 0x32, 0x36, 0x47, 0x30, 0x4E, 0x48, 0x41, 0x53, 0x00, 0x3E, 0x18};

    // 0x73 reponse message
    hifive_info->accessory_state = (accessory_state_t)DEF_ACCESSORY_STATE_MSG;
    printf("Loaded stuff into array, let's print it\n");
    // print out the final struct
    idbus_print_hifive_info(hifive_info);

    return OK;
}

int main()
{
    // board initialization
    stdio_init_all();
    sleep_ms(3000);

    // initialize the idbus_core library
    err = idbus_init();
    if (IDBUS_OK != err)
    {
        printf("Failed to initialize idbus_core library\n");
        ASSERT;
    }

    // prepare a new HiFive instance via idbus_core lib
    err = load_hifive_info(&hifive_info);
    if (OK != err)
    {
        printf("Failed to load local HiFive configuration\n");
        ASSERT;
    }

    // initiate idbus_core lib with the loaded HiFive instance
    err = idbus_init_hifive(&hifive, &hifive_info);
    if (OK != err)
    {
        printf("Failed to initialize HiFive instance\n");
        ASSERT;
    }

    // perform idbus handshake for loaded instance
    err = idbus_do_handshake_hifive(&hifive, (void *)IDBUS_PIN);
    if (OK != err)
    {
        printf("Failed to perform HiFive handshake\n");
        ASSERT;
    }

    printf("DONE");
    while (1)
    {
        sleep_ms(100);
    }
    return IDBUS_OK;
}