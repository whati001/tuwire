/**********************************************************************
 *
 * Filename:    usb_otg.c
 *
 * Description: Sample application how to emulate an usb-otg lightning
 *              adapter. This includes performing the IDBUS and iAP
 *              handshake.
 *
 **********************************************************************/

#include <time.h>
#include <stdio.h>
#include <pico/stdlib.h>

#include "usb_otg.h"
#include "iap_util.h"
#include "iap_cp_gw.h"
#include "iap_hid_gw.h"

#define IAP_GW_ENABLED 0
#define IAP_REQUEST_WIFI_INFO 0
#define IAP_HID_ENABLED 1

#define ASSERT         \
    while (1)          \
    {                  \
        sleep_ms(100); \
    }

// application variables
static int fd = 0, err = 0;
static idbus_hifive_t hifive = {0};

static iap_transport_t iap_transport = {0};

static iap_cp_t iap_cp = {0};

static clock_t start_time, end_time;
// https://lindevs.com/measure-execution-time-of-code-using-raspberry-pi-pico/
void start_time_measurement()
{
    start_time = (clock_t)time_us_64() / 10000;
}
void end_time_measurement(char *label)
{
    end_time = (clock_t)time_us_64() / 10000;
    double duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Duration[%s]: %.8f sec\n", label, duration);
}

/*
 * Perform IDBUS handshake as HiFive chip
 */
int perform_idbus_handshake()
{
    // initialize the idbus_core library
    err = idbus_init();
    CHECK_ERROR(err, "Failed to initialize idbus_core library\n");
    printf("Initialized IDBUS lib properly\n");

    // initiate idbus_core lib with the loaded HiFive instance
    err = idbus_init_hifive(&hifive, &hifive_info);
    CHECK_ERROR(err, "Failed to initialize HiFive instance\n");
    printf("Instantiated HiFive instance properly\n");

    // perform idbus handshake for loaded instance
    err = idbus_do_handshake_hifive(&hifive, (void *)IDBUS_PIN);
    CHECK_ERROR(err, "Failed to perform HiFive handshake\n");

    return err;
}

/*
 * Perform iAP handshake and request USB-Host mode
 */
int perform_iap_handshake()
{
    // open a new UART iAP instance with default config
    err = iap_init_transport(&iap_transport, IAP_UART, NULL);
    CHECK_ERROR(err, "Failed to initialize iap transport\n");
    printf("Initialized iAP transport successfully\n");

    // open a new iAP CP instance with default config
    err = iap_cp_init(&iap_cp, NULL);
    CHECK_ERROR(err, "Failed to initialize iap_cp library\n");
    printf("Initialized iAP CP instance successfully\n");

    // perform authentication handshake
    start_time_measurement();
    err = iap_authenticate_accessory(&iap_transport, &iap_cp);
    CHECK_ERROR(err, "Failed to initialize idbus_core library\n");
    end_time_measurement("iAP-handshake");

    return err;
}

#if 1 == IAP_REQUEST_WIFI_INFO
int request_wifi_credentials()
{
    printf("!!!!!Please unlock the phone via the keyboard\n");
    sleep_ms(10000);

    // try to get the WiFi credentials
    err = iap_request_wifi_credentials(&iap_transport);
    return err;
}
#endif

#if 1 == IAP_GW_ENABLED
int start_iap_gw()
{
    // TODO: find a better solution for this, but we keep it simple now
    printf("Start a iAP-CP gateway to allow the TuWire device to sign a challenge\n");
    uint8_t cancel_handle = 0;
    // open a new iAP CP instance with default config
    err = iap_cp_init(&iap_cp, NULL);
    CHECK_ERROR(err, "Failed to initialize iap_cp library\n");
    printf("Initialized iAP CP instance successfully\n");
    init_iap_cp_gw(&iap_cp);
    run_iap_cp_gw(&cancel_handle);

    return OK;
}
#endif

#if 1 == IAP_HID_ENABLED
int start_iap_hid_hack(iap_transport_t *trans)
{
    int ret = OK;
    printf("Start a iAP-HID gateway to allow a user to control the iDevice\n");
    uint8_t cancel_handle = 0;

    start_time_measurement();
    init_iap_hid_gw(&iap_transport);
    end_time_measurement("HID-init");

    start_time_measurement();
    run_iap_hid_wlan_hack();
    end_time_measurement("WiFi-hack");

    start_time_measurement();
    run_iap_hid_mdm_hack();
    end_time_measurement("MDM-hack");

    // run_iap_hid_gw(&cancel_handle);

    return OK;
}
#endif

int main()
{
    err = OK;
    // board initialization
    stdio_init_all();
    sleep_ms(3000);

    printf("Started to emulate %s adatper\n", __FILE__);

    // perform idbus handshake as HiFive
    err = perform_idbus_handshake();
    printf("Performed IDBUS handshake as HiFive with error code: %d\n", err);
    if (ERR == err)
    {
        goto done;
    }
    TIME_FOR_A_COFFEE;

    // perform iAP handshake to acquire USB-Host mode
    err = perform_iap_handshake();
    printf("Performed iAP handshake as HiFive with error code: %d\n", err);
    if (ERR == err)
    {
        goto done;
    }
    TIME_FOR_A_COFFEE;

    // enable charging
    start_time_measurement();
    err = iap_enable_charging(&iap_transport);
    end_time_measurement("Enable-Charging");
    printf("Enabled charging via iAP as HiFive with error code: %d\n", err);
    if (ERR == err)
    {
        goto done;
    }
    TIME_FOR_A_COFFEE;

#if 1 == IAP_HID_ENABLED
    // err = iap_init_transport(&iap_transport, IAP_UART, NULL);
    err = start_iap_hid_hack(&iap_transport);
    printf("Performed iAP HID emulation as HiFive with error code: %d\n", err);
    if (ERR == err)
    {
        goto done;
    }
#endif

    // request usb-host-mode
    start_time_measurement();
    err = iap_request_usb_host_mode(&iap_transport);
    end_time_measurement("USB-Host");
    printf("Requested USB Host Mode as HiFive with error code: %d\n", err);
    if (ERR == err)
    {
        goto done;
    }
    TIME_FOR_A_COFFEE;

#if 1 == IAP_REQUEST_WIFI_INFO
    err = request_wifi_credentials();
    printf("Requested WiFi Credentials as HiFive with error code: %d\n", err);
    if (ERR == err)
    {
        goto done;
    }
#endif

#if 1 == IAP_GW_ENABLED
    start_iap_gw();
#endif

done:
    ASSERT;
    return err;
}