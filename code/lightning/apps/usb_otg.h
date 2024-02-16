#ifndef USB_OTG_H
#define USB_OTG_H

#include "idbus.h"
#include "iap.h"

/*
 * Define the PIN number used for the IDBUS communication
 * Please ensure, that the breakout board ID pin is connected to this
 * GPIO pin -> GPIO3 in this case
 */
#define IDBUS_PIN 2

// very basic return values, let's reuse what idbus_core uses
#define OK IAP_OK
#define ERR IAP_ERR
#define CHECK_ERROR(err, msg) \
    if (OK != err)            \
    {                         \
        printf(msg);          \
        return err;           \
    }

#define TIME_FOR_A_COFFEE sleep_ms(20);

/*
 * Define HiFive attribute of the USB-OTG adapter
 * It's important to set the proper digitial_id to receive the UART interface
 */
idbus_hifive_info_t hifive_info = {
    .vendor_id = 0x01,
    .product_id = 0x25,
    .revision = 0x01,
    .flags = 0x80,
    .digital_id = {0x11, 0xF0, 0x00, 0x00, 0x00, 0x00},
    // force reboot of iDevice -> see https://github.com/stacksmashing/tamarin-firmware/blob/main/main.c
    //.digital_id = {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00},
    .interface_serial_number = {0xA0, 0x6A, 0x8D, 0x25, 0x26, 0x66},
    .interface_module_number = {0x44, 0x57, 0x48, 0x32, 0x33, 0x38, 0x37, 0x31, 0x45, 0x30, 0x37, 0x46, 0x35, 0x4C, 0x34, 0x41, 0x43, 0x00, 0xAB, 0x88},
    .accessory_serial_number = {0x43, 0x30, 0x38, 0x32, 0x34, 0x32, 0x36, 0x30, 0x31, 0x31, 0x35, 0x44, 0x59, 0x37, 0x51, 0x41, 0x37, 0x00, 0x30, 0x00},
    .accessory_state = DEF_ACCESSORY_STATE_MSG};

#endif