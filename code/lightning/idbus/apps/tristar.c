/**********************************************************************
 *
 * Filename:    tristar.c
 *
 * Description: Simple example, how to emulate the Apple TRISTAR chip
 *
 **********************************************************************/

#include <stdio.h>
#include <pico/stdlib.h>
#include "idbus.h"
#include "util.h"

/*
 * Define the PIN number used for the IDBUS communication
 * Please ensure, that the breakout board ID pin is connected to this
 * GPIO pin -> GPIO2 in this case
 */
#define IDBUS_PIN 2

static int fd = 0, err = 0;
static idbus_tristar_t tristar = {0};
static idbus_tristar_info_t tristar_info = {0};

/*
 * load the Tristar information
 * This example uses hardcoded values, however this could be loaded from disk, network, etc.
 */
int load_tristar_info(idbus_tristar_info_t *tristar_info)
{
    // 0x74 request message
    tristar_info->seven_four_msg = (seven_four_msg_t)DEF_SEVEN_FOUR_MSG;

    // 0x70 request message
    tristar_info->charing_active = (charging_state_msg_t)DEF_CHARGING_ACTIVE_MSG;
    tristar_info->charing_deactivated = (charging_state_msg_t)DEF_CHARGING_DEACTIVATED_MSG;

    // 0x84 request message
    tristar_info->model_number = (model_number_msg_t){.data = {'M', 'N', '9', 'L', '2'}};
    tristar_info->ios_version = (ios_version_msg_t){.data = {'1', '9', 'H', '1', '2'}};

    printf("Loaded stuff into array, let's print it\n");
    // print out the final struct
    idbus_print_tristar_info(tristar_info);

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

    // prepare a new tristar instance via idbus_core lib
    err = load_tristar_info(&tristar_info);
    if (OK != err)
    {
        printf("Failed to load local tristar configuration\n");
        ASSERT;
    }

    // initiate idbus_core lib with the loaded tristar instance
    err = idbus_init_tristar(&tristar, &tristar_info);
    if (OK != err)
    {
        printf("Failed to initialize tristar instance\n");
        ASSERT;
    }

    // perform idbus handshake for loaded instance
    err = idbus_do_handshake_tristar(&tristar, (void *)IDBUS_PIN);
    if (OK != err)
    {
        printf("Failed to perform tristar handshake\n");
        ASSERT;
    }

    printf("DONE");
    while (1)
    {
        sleep_ms(100);
    }
    return IDBUS_OK;
}