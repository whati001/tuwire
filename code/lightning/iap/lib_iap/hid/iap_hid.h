#ifndef IAP_HID_H
#define IAP_HID_H

#include "iap_core.h"

#define IAP_HID_VID 0xAC05        // (little endian) -> real value 0x05AC (Apple)
#define IAP_HID_PID 0xAD12        // (little endian) -> real value 0x12AD
#define IAP_HID_COUNTRY_CODE 0x20 // english

/**
 * @brief HID keyboard button codes.
 * Taken from Zephyr RTOS HID implementation
 */
enum hid_kbd_code
{
    HID_KEY_A = 4,
    HID_KEY_B = 5,
    HID_KEY_C = 6,
    HID_KEY_D = 7,
    HID_KEY_E = 8,
    HID_KEY_F = 9,
    HID_KEY_G = 10,
    HID_KEY_H = 11,
    HID_KEY_I = 12,
    HID_KEY_J = 13,
    HID_KEY_K = 14,
    HID_KEY_L = 15,
    HID_KEY_M = 16,
    HID_KEY_N = 17,
    HID_KEY_O = 18,
    HID_KEY_P = 19,
    HID_KEY_Q = 20,
    HID_KEY_R = 21,
    HID_KEY_S = 22,
    HID_KEY_T = 23,
    HID_KEY_U = 24,
    HID_KEY_V = 25,
    HID_KEY_W = 26,
    HID_KEY_X = 27,
    HID_KEY_Y = 28,
    HID_KEY_Z = 29,
    HID_KEY_1 = 30,
    HID_KEY_2 = 31,
    HID_KEY_3 = 32,
    HID_KEY_4 = 33,
    HID_KEY_5 = 34,
    HID_KEY_6 = 35,
    HID_KEY_7 = 36,
    HID_KEY_8 = 37,
    HID_KEY_9 = 38,
    HID_KEY_0 = 39,
    HID_KEY_ENTER = 40,
    HID_KEY_ESC = 41,
    HID_KEY_BACKSPACE = 42,
    HID_KEY_TAB = 43,
    HID_KEY_SPACE = 44,
    HID_KEY_MINUS = 45,
    HID_KEY_EQUAL = 46,
    HID_KEY_LEFTBRACE = 47,
    HID_KEY_RIGHTBRACE = 48,
    HID_KEY_BACKSLASH = 49,
    HID_KEY_HASH = 50, /* Non-US # and ~ */
    HID_KEY_SEMICOLON = 51,
    HID_KEY_APOSTROPHE = 52,
    HID_KEY_GRAVE = 53,
    HID_KEY_COMMA = 54,
    HID_KEY_DOT = 55,
    HID_KEY_SLASH = 56,
    HID_KEY_CAPSLOCK = 57,
    HID_KEY_F1 = 58,
    HID_KEY_F2 = 59,
    HID_KEY_F3 = 60,
    HID_KEY_F4 = 61,
    HID_KEY_F5 = 62,
    HID_KEY_F6 = 63,
    HID_KEY_F7 = 64,
    HID_KEY_F8 = 65,
    HID_KEY_F9 = 66,
    HID_KEY_F10 = 67,
    HID_KEY_F11 = 68,
    HID_KEY_F12 = 69,
    HID_KEY_SYSRQ = 70, /* PRINTSCREEN */
    HID_KEY_SCROLLLOCK = 71,
    HID_KEY_PAUSE = 72,
    HID_KEY_INSERT = 73,
    HID_KEY_HOME = 74,
    HID_KEY_PAGEUP = 75,
    HID_KEY_DELETE = 76,
    HID_KEY_END = 77,
    HID_KEY_PAGEDOWN = 78,
    HID_KEY_RIGHT = 79,
    HID_KEY_LEFT = 80,
    HID_KEY_DOWN = 81,
    HID_KEY_UP = 82,
    HID_KEY_NUMLOCK = 83,
    HID_KEY_KPSLASH = 84,    /* NUMPAD DIVIDE */
    HID_KEY_KPASTERISK = 85, /* NUMPAD MULTIPLY */
    HID_KEY_KPMINUS = 86,
    HID_KEY_KPPLUS = 87,
    HID_KEY_KPENTER = 88,
    HID_KEY_KP_1 = 89,
    HID_KEY_KP_2 = 90,
    HID_KEY_KP_3 = 91,
    HID_KEY_KP_4 = 92,
    HID_KEY_KP_5 = 93,
    HID_KEY_KP_6 = 94,
    HID_KEY_KP_7 = 95,
    HID_KEY_KP_8 = 96,
    HID_KEY_KP_9 = 97,
    HID_KEY_KP_0 = 98,
};

/**
 * @brief HID keyboard modifiers.
 * Taken from Zephyr RTOS HID implementation
 */
enum hid_kbd_modifier
{
    HID_KBD_MODIFIER_NONE = 0x00,
    HID_KBD_MODIFIER_LEFT_CTRL = 0x01,
    HID_KBD_MODIFIER_LEFT_SHIFT = 0x02,
    HID_KBD_MODIFIER_LEFT_ALT = 0x04,
    HID_KBD_MODIFIER_LEFT_UI = 0x08,
    HID_KBD_MODIFIER_RIGHT_CTRL = 0x10,
    HID_KBD_MODIFIER_RIGHT_SHIFT = 0x20,
    HID_KBD_MODIFIER_RIGHT_ALT = 0x40,
    HID_KBD_MODIFIER_RIGHT_UI = 0x80,
};

/*
 * Open new iAP HID keyboard instance
 */
int iap_hid_open_keyboard(iap_transport_t *trans, uint8_t *hid_idx);

/*
 * Open new iAP HID mouse instance
 */
int iap_hid_open_mouse(iap_transport_t *trans, uint8_t *hid_idx);

int iap_hid_open_control(iap_transport_t *trans, uint8_t *hid_idx);
/*
 * Send a new HID report for the previously opened hid_idx through the specified iap_transport
 */
int iap_hid_send_report(iap_transport_t *trans, uint8_t hid_idx, uint8_t *report, uint8_t report_size);

int iap_hid_send_kbd_clear(iap_transport_t *trans, uint8_t hid_idx);

int iap_hid_send_kbd_char(iap_transport_t *trans, uint8_t hid_idx, char chr);

int iap_hid_send_mouse_clear(iap_transport_t *trans, uint8_t hid_idx);

int iap_hid_send_mouse_move_up(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_move_down(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_move_left(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_move_right(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_click_left(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_click_right(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_scroll_up(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_scroll_down(iap_transport_t *trans, uint8_t hid_idx);
int iap_hid_send_mouse_move_origin(iap_transport_t *trans, uint8_t hid_idx);

int iap_hid_send_home_btn(iap_transport_t *trans, uint8_t hid_idx);
#endif