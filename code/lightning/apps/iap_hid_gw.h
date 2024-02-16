#ifndef IAP_HID_GW_H
#define IAP_HID_GW_H

#include <stdint.h>
#include "iap.h"

#define HID_MOUSE_MOVE_UP 'W'
#define HID_MOUSE_MOVE_DOWN 'S'
#define HID_MOUSE_MOVE_LEFT 'D'
#define HID_MOUSE_MOVE_RIGHT 'A'
#define HID_MOUSE_CLICK_LEFT 'Q'
#define HID_MOUSE_CLICK_RIGHT 'E'
#define HID_MOUSE_SCROLL_DOWN 'R'
#define HID_MOUSE_SCROLL_UP 'T'

int init_iap_hid_gw(iap_transport_t *trans);

void run_iap_hid_wlan_hack();

void run_iap_hid_mdm_hack();

void run_iap_hid_gw(uint8_t *cancel);

#endif