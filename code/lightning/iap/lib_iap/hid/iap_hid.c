
#include <stdio.h>
#include <string.h>
#include "iap_hid.h"
#include "iap_util.h"
// http://www.freebsddiary.org/APC/usb_hid_usages.php

// keyboard hid report captured with wireshark from a dell keyboard
static uint8_t hid_report_kbd_size = 63;
static uint8_t hid_report_kbd[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07,
    0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x75, 0x08,
    0x95, 0x01, 0x81, 0x03, 0x75, 0x01, 0x95, 0x05,
    0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x75, 0x03, 0x95, 0x01, 0x91, 0x03, 0x75, 0x08,
    0x95, 0x06, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07,
    0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0};
// example keyboard report:
//  - clear:  [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

static uint8_t hid_report_mouse_size = 52;
static uint8_t hid_report_mouse[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01,
    0xa1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x02,
    0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x02,
    0x81, 0x02, 0x75, 0x06, 0x95, 0x01, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03,
    0x81, 0x06, 0xc0, 0xc0};
// example mouse report: [btn, X, Y, ??]
//  - btn left:  [0x01, 0x00, 0x00, 0x00]
//  - btn right: [0x02, 0x00, 0x00, 0x00]
//  - left (-X): [0x00, 0xE0, 0x00, 0x00] // 0xE0 -> singed minus value
//  - left (+X): [0x00, 0x20, 0x00, 0x00]
//  - left (-Y): [0x00, 0x00, 0x20, 0x00]
//  - left (+Y): [0x00, 0x00, 0xE0, 0x00] // 0xE0 -> singed minus value
//  - scroll - : [0x00, 0x00, 0x00, 0x20]
//  - scroll + : [0x00, 0x00, 0x00, 0xE0]
//  - clear:     [0x00, 0x00, 0x00, 0x00]

static uint8_t hid_report_control_size = 109;
static uint8_t hid_report_control[] = {
    0x05, 0x01, 0x09, 0x80, 0xa1, 0x01, 0x85, 0x01,
    0x19, 0x81, 0x29, 0x83, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x03, 0x81, 0x02, 0x75, 0x05,
    0x95, 0x01, 0x81, 0x01, 0xc0, 0x05, 0x0c, 0x09,
    0x01, 0xa1, 0x01, 0x85, 0x02, 0x0a, 0xb5, 0x00,
    0x0a, 0xb6, 0x00, 0x0a, 0xb7, 0x00, 0x0a, 0x83,
    0x01, 0x0a, 0xcd, 0x00, 0x0a, 0xe9, 0x00, 0x0a,
    0xea, 0x00, 0x0a, 0x40, 0x00, 0x0a, 0x24, 0x02,
    0x0a, 0x8a, 0x01, 0x0a, 0x25, 0x02, 0x0a, 0x23,
    0x02, 0x0a, 0x21, 0x02, 0x0a, 0x26, 0x02, 0x0a,
    0x27, 0x02, 0x0a, 0x2a, 0x02, 0x0a, 0x92, 0x01,
    0x0a, 0x94, 0x01, 0x09, 0xb8, 0x0a, 0xa7, 0x01,
    0x75, 0x01, 0x95, 0x14, 0x81, 0x02, 0x75, 0x01,
    0x95, 0x04, 0x81, 0x01, 0xc0};
// example:
//  - home: [0x02, 0x00, 0x01, 0x00]

static uint8_t usb_idx = 0;

// acquire some memory to perform the iAP handshake
static uint8_t req_buf[265] = {0};
static uint8_t res_buf[265] = {0};
static iap_command_t req = {
    .lingoid = IAP_LINGO_SIMPLE_REMOTE,
    .command = 0x00,
    .param = req_buf,
    .param_size = 0};

static iap_command_t res = {
    .lingoid = IAP_LINGO_SIMPLE_REMOTE,
    .command = 0x00,
    .param = res_buf,
    .param_size = 0};

static uint8_t needs_shift(uint8_t ascii)
{
    if ((ascii < 33) || (ascii == 39U))
    {
        return 0;
    }
    else if ((ascii >= 33U) && (ascii < 44))
    {
        return 1;
    }
    else if ((ascii >= 44U) && (ascii < 58))
    {
        return 0;
    }
    else if ((ascii == 59U) || (ascii == 61U))
    {
        return 0;
    }
    else if ((ascii >= 58U) && (ascii < 91))
    {
        return 1;
    }
    else if ((ascii >= 91U) && (ascii < 94))
    {
        return 0;
    }
    else if ((ascii == 94U) || (ascii == 95U))
    {
        return 1;
    }
    else if ((ascii > 95) && (ascii < 123))
    {
        return 0;
    }
    else if ((ascii > 122) && (ascii < 127))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int ascii_to_hid(uint8_t ascii)
{
    if (ascii < 48)
    {
        /* Special characters */
        switch (ascii)
        {
        case 13:
            return HID_KEY_ENTER;
        case 32:
            return HID_KEY_SPACE;
        case 33:
            return HID_KEY_1;
        case 34:
            return HID_KEY_APOSTROPHE;
        case 35:
            return HID_KEY_3;
        case 36:
            return HID_KEY_4;
        case 37:
            return HID_KEY_5;
        case 38:
            return HID_KEY_7;
        case 39:
            return HID_KEY_APOSTROPHE;
        case 40:
            return HID_KEY_9;
        case 41:
            return HID_KEY_0;
        case 42:
            return HID_KEY_8;
        case 43:
            return HID_KEY_EQUAL;
        case 44:
            return HID_KEY_COMMA;
        case 45:
            return HID_KEY_MINUS;
        case 46:
            return HID_KEY_DOT;
        case 47:
            return HID_KEY_SLASH;
        default:
            return -1;
        }
    }
    else if (ascii < 58)
    {
        /* Numbers */
        if (ascii == 48U)
        {
            return HID_KEY_0;
        }
        else
        {
            return ascii - 19;
        }
    }
    else if (ascii < 65)
    {
        /* Special characters #2 */
        switch (ascii)
        {
        case 58:
            return HID_KEY_SEMICOLON;
        case 59:
            return HID_KEY_SEMICOLON;
        case 60:
            return HID_KEY_COMMA;
        case 61:
            return HID_KEY_EQUAL;
        case 62:
            return HID_KEY_DOT;
        case 63:
            return HID_KEY_SLASH;
        case 64:
            return HID_KEY_2;
        default:
            return -1;
        }
    }
    else if (ascii < 91)
    {
        /* Uppercase characters */
        return ascii - 61U;
    }
    else if (ascii < 97)
    {
        /* Special characters #3 */
        switch (ascii)
        {
        case 91:
            return HID_KEY_LEFTBRACE;
        case 92:
            return HID_KEY_BACKSLASH;
        case 93:
            return HID_KEY_RIGHTBRACE;
        case 94:
            return HID_KEY_6;
        case 95:
            return HID_KEY_MINUS;
        case 96:
            return HID_KEY_GRAVE;
        default:
            return -1;
        }
    }
    else if (ascii < 123)
    {
        /* Lowercase letters */
        return ascii - 93;
    }
    else if (ascii < 128)
    {
        /* Special characters #4 */
        switch (ascii)
        {
        case 123:
            return HID_KEY_LEFTBRACE;
        case 124:
            return HID_KEY_BACKSLASH;
        case 125:
            return HID_KEY_RIGHTBRACE;
        case 126:
            return HID_KEY_GRAVE;
        case 127:
            return HID_KEY_BACKSPACE;
        default:
            return -1;
        }
    }

    return -1;
}

static int iap_hid_open_device(iap_transport_t *trans, uint8_t *hid_idx, uint8_t *report, uint8_t report_size)
{
    int ret = IAP_OK;
    iap_cmd_registerdesc_t *param = (iap_cmd_registerdesc_t *)req.param;

    req.lingoid = IAP_LINGO_SIMPLE_REMOTE;
    req.command = IAP_CMD_REGISTERDESC;
    req.param_size = report_size + sizeof(iap_cmd_registerdesc_t);

    param->index = ++usb_idx;
    param->vid = IAP_HID_VID;
    param->pid = IAP_HID_PID;
    param->country_code = IAP_HID_COUNTRY_CODE; // US country code: iAP table Table 4-31
    memcpy(param->hid_descriptor, report, report_size);

    iap_increment_transid(trans);
    ret = iap_send_command(trans, &req, &res);
    CHECK_ERR(ret, "Failed to send iAP command to register a new HID device\n");
    printf("Successfully registered a new iAP HID device\n");

    // report back the
    *hid_idx = usb_idx;

    return ret;
}

int iap_hid_open_keyboard(iap_transport_t *trans, uint8_t *hid_idx)
{
    return iap_hid_open_device(trans, hid_idx, hid_report_kbd, hid_report_kbd_size);
}

int iap_hid_open_mouse(iap_transport_t *trans, uint8_t *hid_idx)
{
    return iap_hid_open_device(trans, hid_idx, hid_report_mouse, hid_report_mouse_size);
}

int iap_hid_open_control(iap_transport_t *trans, uint8_t *hid_idx)
{
    return iap_hid_open_device(trans, hid_idx, hid_report_control, hid_report_control_size);
}

int iap_hid_send_report(iap_transport_t *trans, uint8_t hid_idx, uint8_t *report, uint8_t report_size)
{
    int ret = IAP_OK;
    iap_cmd_acchidreport_t *param = (iap_cmd_acchidreport_t *)req.param;

    req.lingoid = IAP_LINGO_SIMPLE_REMOTE;
    req.command = IAP_CMD_IPODHIDREPORT;
    req.param_size = report_size + sizeof(iap_cmd_acchidreport_t);

    param->index = hid_idx;
    param->report_type = 0;
    memcpy(param->report, report, report_size);

    // iap_increment_transid(trans);
    ret = iap_send_command(trans, &req, &res);
    CHECK_ERR(ret, "Failed to send iAP command sending a new HID report\n");
    printf("Successfully send iAP HID report: [private] with index: %d\n", hid_idx);

    return ret;
}

int iap_hid_send_kbd_clear(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return iap_hid_send_report(trans, hid_idx, rep, 8);
}

int iap_hid_send_kbd_char(iap_transport_t *trans, uint8_t hid_idx, char chr)
{
    uint8_t rep[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t ch = ascii_to_hid(chr);

    if (ch == ((uint8_t)-1))
    {
        printf("Unsupported character: 0x%x\n", chr);
        return IAP_ERR;
    }
    printf("ASCII code: 0x%x mapped to HID code: 0x%x\n", chr, ch);
    rep[7] = ch;

    if (needs_shift(chr))
    {
        rep[0] |= HID_KBD_MODIFIER_RIGHT_SHIFT;
    }

    int ret = iap_hid_send_report(trans, hid_idx, rep, 8);
    ret |= iap_hid_send_kbd_clear(trans, hid_idx);

    return ret;
}

int iap_hid_send_mouse_clear(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x00, 0x00, 0x00};
    return iap_hid_send_report(trans, hid_idx, rep, 4);
}

int iap_hid_send_mouse_move_up(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x00, 0xF0, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);
    return ret;
}

int iap_hid_send_mouse_move_down(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x00, 0x10, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);
    return ret;
}

int iap_hid_send_mouse_move_left(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x10, 0x00, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);
    return ret;
}

int iap_hid_send_mouse_move_right(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0xF0, 0x00, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);
    return ret;
}

int iap_hid_send_mouse_click_left(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x01, 0x00, 0x00, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);
    return ret;
}

int iap_hid_send_mouse_click_right(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x02, 0x00, 0x00, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);
    return ret;
}

int iap_hid_send_mouse_scroll_down(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x00, 0x00, 0x70};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    return ret;
}

int iap_hid_send_mouse_scroll_up(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x02, 0x00, 0x00, 0x80};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    return ret;
}

int iap_hid_send_mouse_move_origin(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x00, 0x80, 0x70, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_mouse_clear(trans, hid_idx);

    return ret;
}

static int iap_hid_send_control_clear(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x02, 0x00, 0x00, 0x00};

    return iap_hid_send_report(trans, hid_idx, rep, 4);
}

int iap_hid_send_home_btn(iap_transport_t *trans, uint8_t hid_idx)
{
    uint8_t rep[] = {0x02, 0x80, 0x00, 0x00};

    int ret = iap_hid_send_report(trans, hid_idx, rep, 4);
    ret |= iap_hid_send_control_clear(trans, hid_idx);
    return ret;
}