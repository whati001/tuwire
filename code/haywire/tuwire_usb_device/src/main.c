/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>

LOG_MODULE_REGISTER(main);

// overwrite the weak serial number computation function
// we desire the value in CONFIG_USB_DEVICE_SN as serial number
uint8_t *usb_update_sn_string_descriptor(void)
{
	static uint8_t sn[sizeof(CONFIG_USB_DEVICE_SN) + 1];
	memset(sn, 0, sizeof(sn));
	memcpy(sn, CONFIG_USB_DEVICE_SN, sizeof(CONFIG_USB_DEVICE_SN));

	return sn;
}

void main(void)
{
	int ret;

	ret = usb_enable(NULL);
	if (ret != 0)
	{
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("The device is put in USB mass storage mode.\n");
}
