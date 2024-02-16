
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iap_core.h"
#include "iap_util.h"

void iap_compute_checksum(iap_msg_t *msg)
{
    iap_checksum_t checksum = 0;

    checksum += msg->length;
    checksum += msg->command->lingoid;
    checksum += msg->command->command;
    checksum += msg->transid;
    for (uint32_t idx = 0; idx < msg->command->param_size; idx++)
    {
        checksum += msg->command->param[idx];
    }

    checksum = (0x100 - checksum);

    msg->checksum = checksum;
}

// TODO: check overflow
int _iap_serialize_msg(iap_transport_t *trans, iap_msg_t *msg)
{
    // get the write buffer
    trans->write_buf_len = 0;
    uint8_t *buf_ptr = trans->write_buf;

    // add the header
    *buf_ptr++ = msg->header_msb;
    if (IAP_UART == trans->mode)
    {
        *buf_ptr++ = msg->header_lsb;
    }

    // add the length
    if (msg->length >= 0xFD)
    {
        *buf_ptr++ = IAP_LONG_LENGTH_MARKER;
        *buf_ptr++ = (msg->length & 0xFF00);
    }
    *buf_ptr++ = (msg->length & 0x00FF);

    // add the lingoid
    *buf_ptr++ = msg->command->lingoid;

    // add the command
    *buf_ptr++ = msg->command->command;

    // add the transid
    *buf_ptr++ = (trans->transid & 0xFF00) >> 8;
    *buf_ptr++ = trans->transid & 0x00FF;

    // add the param
    memcpy(buf_ptr, msg->command->param, msg->command->param_size);
    buf_ptr += msg->command->param_size;

    // add the checksum
    *buf_ptr++ = msg->checksum;

    // correct the buffer size
    trans->write_buf_len = buf_ptr - trans->write_buf;

    return IAP_OK;
}

int _iap_deserialize_msg(iap_transport_t *trans, iap_msg_t *msg)
{
    // get the read buffer
    uint8_t *buf_ptr = trans->read_buf;

    if (trans->read_buf_len < IAP_MSG_MIN_SIZE)
    {
        printf("Failed to deserialize iAP message, overall size to small maybe some recv error\n");
        return IAP_ERR;
    }

    // parse the header value
    msg->header_msb = *buf_ptr++;
    if (IAP_UART == trans->mode)
    {
        msg->header_lsb = *buf_ptr++;
    }

    // parse length
    msg->length = *buf_ptr++;
    if (IAP_LONG_LENGTH_MARKER == msg->length)
    {
        printf("Received a 3-byte length iAP message, start parsing it\n");
        msg->length = (*buf_ptr++) << 8;
        msg->length |= (*buf_ptr++);
    }
    // validate length now
    if (msg->length == (buf_ptr - trans->read_buf))
    {
        printf("Received invalid amount of bytes, length stated in iAP msg does not match up with the remaining available bytes: \n");
        printf("  - msg.length : 0x%04x (lingoId + cmdId + cmdParams + checksum)\n", msg->length);
        printf("  - trans.buf remaining: 0x%04x\n", (buf_ptr - trans->read_buf));
        return IAP_ERR;
    }

    // parse lingoid
    msg->command->lingoid = *buf_ptr++;

    // parse command
    msg->command->command = *buf_ptr++;

    // parse transid
    msg->transid = (*buf_ptr++) << 8;
    msg->transid |= (*buf_ptr++);

    // parse param; length equals to the length minus the (lingoId + commandId + transId)
    msg->command->param_size = (msg->length - IAP_LINGOID_SIZE - IAP_COMMAND_SIZE - IAP_TRANSID_SIZE);
    memcpy(msg->command->param, buf_ptr, msg->command->param_size);
    buf_ptr += msg->command->param_size;

    // parse/validate checksum
    iap_checksum_t recv_checksum = *buf_ptr;
    iap_compute_checksum(msg);
    // validate checksum
    if (msg->checksum != recv_checksum)
    {
        printf("Invalid checksum received:\n  computed: 0x%x\n  received: 0x%x\n", msg->checksum, recv_checksum);
        return IAP_ERR;
    }

    return IAP_OK;
}

int iap_send_msg(iap_transport_t *trans, iap_msg_t *req_msg, iap_msg_t *res_msg)
{
    int err = IAP_OK;
    // serialize the message into the transport buffer
    _iap_serialize_msg(trans, req_msg);
    CHECK_ERR(err, "Failed to serialize iAP message\n");
    printf("Send out data: ");
    PRINT_ARRAY(trans->write_buf, trans->write_buf_len);

    // send out the data and wait if needed for the response
    err = iap_transfer_out(trans, (res_msg != NULL));
    CHECK_ERR(err, "Failed to send out iAP message\n");

    if (NULL == res_msg)
    {
        printf("No iAP response message requested, skip checking and deserializing it\n");
        return IAP_OK;
    }

    // deserialize the response message
    printf("Received data:");
    PRINT_ARRAY(trans->read_buf, trans->read_buf_len);
    err = _iap_deserialize_msg(trans, res_msg);
    CHECK_ERR(err, "Failed to deserialize received response\n");

    return IAP_OK;
}

int iap_recv_msg(iap_transport_t *trans, iap_msg_t *req_msg, iap_msg_t *res_msg)
{
    int err = IAP_OK;

    if (NULL != res_msg)
    {
        err = _iap_serialize_msg(trans, res_msg);
        CHECK_ERR(err, "Failed to serialize iAP message\n");
    }

    err = iap_transfer_in(trans, (res_msg != NULL));
    CHECK_ERR(err, "Failed to receive some iAP message\n");

    printf("Received data:");
    PRINT_ARRAY(trans->read_buf, trans->read_buf_len);
    if (NULL != res_msg)
    {
        printf("Send data:");
        PRINT_ARRAY(trans->write_buf, trans->write_buf_len);
    }

    err = _iap_deserialize_msg(trans, req_msg);
    CHECK_ERR(err, "Failed to deserialize received response\n");

    return IAP_OK;
}

int iap_send_command(iap_transport_t *trans, iap_command_t *req_cmd, iap_command_t *res_cmd)
{
    int err = IAP_OK;

    iap_msg_t req_msg = {
        .header_msb = IAP_HEADER_MSB,
        .header_lsb = IAP_HEADER_LSB,
        .transid = trans->transid,
        .length = (req_cmd->param_size +
                   IAP_LINGOID_SIZE +
                   IAP_COMMAND_SIZE +
                   IAP_TRANSID_SIZE),
        .checksum = 0,
        .command = req_cmd};
    if (IAP_UART != trans->mode)
    {
        req_msg.header_msb = req_msg.header_lsb;
    }

    iap_compute_checksum(&req_msg);

    if (NULL == res_cmd)
    {
        return iap_send_msg(trans, &req_msg, NULL);
    }

    iap_msg_t res_msg = {0};
    res_msg.command = res_cmd;

    return iap_send_msg(trans, &req_msg, &res_msg);
}

int iap_recv_command(iap_transport_t *trans, iap_command_t *req_cmd, iap_command_t *res_cmd)
{
    int err = IAP_OK;

    iap_msg_t req_msg = {0};
    req_msg.command = req_cmd;

    if (NULL == res_cmd)
    {
        return iap_recv_msg(trans, &req_msg, NULL);
    }

    iap_msg_t res_msg = {
        .header_msb = IAP_HEADER_MSB,
        .header_lsb = IAP_HEADER_LSB,
        .transid = trans->transid,
        .length = (req_cmd->param_size +
                   IAP_LINGOID_SIZE +
                   IAP_COMMAND_SIZE +
                   IAP_TRANSID_SIZE),
        .checksum = 0,
        .command = res_cmd};
    iap_compute_checksum(&res_msg);

    return iap_recv_msg(trans, &req_msg, &res_msg);
}

static int _iap_handle_idps(iap_transport_t *trans, iap_command_t *req, iap_command_t *res)
{
    int err = 0;
    // reset transport instance
    trans->authenticated = 0;
    trans->max_packet_size = 0;
    trans->idps_done = 0;

    // StartIDPS
    req->lingoid = IAP_LINGO_GENERAL;
    req->command = IAP_CMD_START_IDPS;
    err = iap_send_command(trans, req, res);
    CHECK_ERR(err, "Failed to send StartIDPS message\n");
    CHECK_IPOD_ACK(res, "StartIDPS command not acknowledged by iDevice");

    // increment the transaction id
    iap_increment_transid(trans);

    // RequestTransportMaxPayloadSize
    req->lingoid = IAP_LINGO_GENERAL;
    req->command = IAP_CMD_REQMAXTRANSSIZE;
    err = iap_send_command(trans, req, res);
    CHECK_ERR(err, "Failed to send RequestTransportMaxPayloadSize message\n");
    // ReturnTransportMaxPayloadSize
    if (IAP_CMD_RESMAXTRANSSIZE == res->command)
    {
        trans->max_packet_size = res->param[0] << 8;
        trans->max_packet_size |= res->param[1];
        debug("Parsed IDPS (transport) max packet size: 0x%04x\n", trans->max_packet_size);
    }

    // increment the transaction id
    iap_increment_transid(trans);

    // GetiPodOptionsForLingo
    req->lingoid = IAP_LINGO_GENERAL;
    req->command = IAP_CMD_GETIPODOPTIONLINGO;
    err = iap_send_command(trans, req, res);
    CHECK_ERR(err, "Failed to send GetiPodOptionsForLingo message\n");

    // // haywire stuff ????
    // // increment the transaction id
    // iap_increment_transid(trans);
    // req->lingoid = IAP_LINGO_GENERAL;
    // req->command = 0x5a;
    // err = iap_send_command(trans, req, res);
    // PRINT_IAP_CMD(res);

    // increment the transaction id
    iap_increment_transid(trans);

    // SetFIDTokenValues
    req->lingoid = IAP_LINGO_GENERAL;
    req->command = IAP_CMD_SETSFIDTOKENVALUE;
    req->param_size = 77;
    memcpy(req->param, (uint8_t[]){0x07, 0x0F, 0x00, 0x00, 0x05, 0x00, 0x02, 0x03, 0x04, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x02, 0x01, 0x43, 0x63, 0x63, 0x2D, 0x63, 0x68, 0x69, 0x70, 0x00, 0x06, 0x00, 0x02, 0x04, 0x01, 0x01, 0x02, 0x06, 0x00, 0x02, 0x05, 0x02, 0x03, 0x01, 0x0B, 0x00, 0x02, 0x06, 0x58, 0x6C, 0x73, 0x54, 0x45, 0x43, 0x48, 0x00, 0x08, 0x00, 0x02, 0x07, 0x43, 0x30, 0x32, 0x00, 0x00}, req->param_size);
    err = iap_send_command(trans, req, res);
    CHECK_ERR(err, "Failed to send SetFIDTokenValues message\n");
    // TODO: implement some check here, I am to lazy so I will expect to get an ACK :p

    // increment the transaction id
    iap_increment_transid(trans);

    // EndIDPS
    req->lingoid = IAP_LINGO_GENERAL;
    req->command = IAP_CMD_ENDIDPS;
    req->param_size = 1;
    memcpy(req->param, (uint8_t[]){0x00}, 1);
    err = iap_send_command(trans, req, res);
    CHECK_ERR(err, "Failed to send EndIDPS message\n");

    return ((iap_cmd_idpsstatus_t *)res->param)->status;
}

int _iap_send_certificate(iap_transport_t *trans, iap_cp_t *cp, iap_command_t *req, iap_command_t *res)
{
    int err = IAP_OK;
    uint16_t cert_size = 0;

    // load the certificate size
    err = iap_cp_read_cert_size(cp, &cert_size);
    CHECK_ERR(err, "Failed to retrieve the certificate size\n");

    // create a new transaction id
    iap_increment_transid(trans);

    // paginate the cert information to the iDevice
    printf("Lets start to send over the cert page by page to the iDevice, total length: 0x%04x\n", cert_size);
    req->lingoid = IAP_LINGO_GENERAL;
    req->command = IAP_CMD_RETACCAUTHINFO;

    uint16_t page_size = 0;
    uint8_t section_idx = 0;
    uint8_t section_count = iap_ceil(((float)cert_size) / ((float)IAP_CP_REG_CERT_DATA_LEN)) - 1;
    while (cert_size > 0)
    {
        // Table 3-28 RetAccessoryAuthenticationInfo packet, Authentication 2.
        // construct params as stated below:
        // add the metadata
        uint8_t *param_ptr = req->param;
        *param_ptr++ = cp->major;
        *param_ptr++ = cp->minor;
        *param_ptr++ = section_idx;
        *param_ptr++ = section_count;

        // add the actual cert binary data
        page_size = (uint8_t)IAP_MIN(IAP_CP_REG_CERT_DATA_LEN, cert_size);
        err = iap_cp_read_cert_page(cp, param_ptr, page_size);
        CHECK_ERR(err, "Failed to retrieve certificate data page\n");

        // correct param_size and send page to iDevice
        req->param_size = page_size + 4;
        err = iap_send_command(trans, req, res);
        CHECK_ERR(err, "Failed to send certificate page to iDevice\n");
        // besides the last iteration, we receive 0x01 - iPodAcks
        // in the last iteration, we receive an 0x16 - AckAccessoryAuthenticationInfo
        // because both contain only a single byte and 0x00 is signals success, we can reuse the macro below
        CHECK_IPOD_ACK(res, "iDevice has rejected the certificate page\n");

        cert_size -= page_size;
        section_idx++;
    }

    return IAP_OK;
}

static int _iap_handle_acc_auth(iap_transport_t *trans, iap_cp_t *cp, iap_command_t *req, iap_command_t *res)
{
    int err = IAP_OK;

    err = iap_recv_command(trans, req, NULL);

    // check if we have received a 0x
    if (req->lingoid == IAP_LINGO_GENERAL && req->command == IAP_CMD_GETACCAUTHINFO)
    {
        printf("Received GetAccessoryAuthenticationInfo, let's send the cert data\n");

        err = _iap_send_certificate(trans, cp, req, res);
        CHECK_ERR(err, "Failed to send certificate data to iDevice\n");
        printf("Successfully send certificate data\n");

        // read the challenge
        printf("Start to read the challenge send by the iDevice\n");
        err = iap_recv_command(trans, req, NULL);
        CHECK_ERR(err, "Failed to receive auth challenge from iDevice\n");
        printf("Successfully read challenge\n");

        // sign the challenge; we will misuse the req and res buffers
        err = iap_cp_sign_challenge(cp, req->param, IAP_CP_CHALLENGE_SIZE, res->param, (uint16_t *)&res->param_size);
        CHECK_ERR(err, "Failed to generate the challenge signature\n");
        printf("Successfully signed challenge\n");

        // new transactionId
        iap_increment_transid(trans);

        // send the challenge back to the iDevice
        req->lingoid = IAP_LINGO_GENERAL;
        req->command = IAP_CMD_RETACCAUTHSIG;
        // copy over the signature into the req buffer
        req->param_size = res->param_size;
        memcpy(req->param, res->param, req->param_size);
        err = iap_send_command(trans, req, res);
        CHECK_ERR(err, "Failed to send the challenge signature to the iDevice\n");
        // not sure why, but the iPhone6 attaches a 4.6.3 Command 0x04: NotifyUSBMode message
        // so if we receive it, simply ignore it and wait for the real 3.3.22 Command 0x19: AckAccessoryAuthenticationStatus
        if (res->lingoid == IAP_LINGO_USB_HOST_MODE && IAP_CMD_NOTIFYUSBMODE)
        {
            printf("Received USBMode-Notification, let's ignore it and return to reading\n");
            err = iap_recv_command(trans, res, NULL);
        }
        // 3.3.22 Command 0x19: AckAccessoryAuthenticationStatus -> !0x00 in params signals error
        CHECK_IPOD_ACK(res, "iDevice has rejected the challenge signature\n");

        return IAP_OK;
    }

    return IAP_ERR;
}

int iap_authenticate_accessory(iap_transport_t *trans, iap_cp_t *cp)
{
    int err = IAP_OK;

    // acquire some memory to perform the iAP handshake
    uint8_t req_buf[265] = {0};
    uint8_t res_buf[265] = {0};
    iap_command_t req = {
        .lingoid = IAP_LINGO_GENERAL,
        .command = 0x00,
        .param = req_buf,
        .param_size = 0};

    iap_command_t res = {
        .lingoid = IAP_LINGO_GENERAL,
        .command = 0x00,
        .param = res_buf,
        .param_size = 0};

    printf("Start to perform iAP1 accessory authentication process\n");

    // perform IDPS transport handshake
    err = _iap_handle_idps(trans, &req, &res);
    CHECK_ERR(err, "Failed to perform IDPS Handshake\n");
    printf("Finished to perform IDPS transport handshake\n");

    // perform accessory authentication
    err = _iap_handle_acc_auth(trans, cp, &req, &res);
    CHECK_ERR(err, "Failed to perform Accessory Authenticate Handshake\n");

    trans->authenticated = 1;

    printf("Finished to perform iAP1 accessory authentication process\n");
    return IAP_OK;
}

int iap_enable_charging(iap_transport_t *trans)
{
    int err = IAP_OK;

    // acquire some memory to perform the iAP handshake
    uint8_t res_buf[265] = {0};
    iap_command_t req = {
        .lingoid = IAP_LINGO_GENERAL,
        .command = IAP_CMD_SETAVAILABLECURRENT,
        .param = (uint8_t[]){0x08, 0x34},
        .param_size = 2};

    iap_command_t res = {
        .lingoid = IAP_LINGO_GENERAL,
        .command = 0x00,
        .param = res_buf,
        .param_size = 0};

    printf("Start to activate charging Apple device via iAP\n");

    // set current
    iap_increment_transid(trans);
    err = iap_send_command(trans, &req, &res);
    CHECK_ERR(err, "Failed to request USB Host mode\n");
    CHECK_IPOD_ACK((&res), "iDevice declined to switch into USB Host Mode");

    // request to charge
    req.command = IAP_CMD_SETCHARGINGSTATE;
    req.param = (uint8_t[]){0x01};
    req.param_size = 1;
    iap_increment_transid(trans);
    err = iap_send_command(trans, &req, &res);
    CHECK_ERR(err, "Failed to request USB Host mode\n");
    CHECK_IPOD_ACK((&res), "iDevice declined to switch into USB Host Mode");
    iap_increment_transid(trans);
}

int iap_request_usb_host_mode(iap_transport_t *trans)
{
    int err = IAP_OK;

    // acquire some memory to perform the iAP handshake
    uint8_t res_buf[265] = {0};
    iap_command_t req = {
        .lingoid = IAP_LINGO_USB_HOST_MODE,
        .command = IAP_CMD_SETIPODUBSMODE,
        .param = (uint8_t[]){USB_MODE_HOST},
        .param_size = 1};

    iap_command_t res = {
        .lingoid = IAP_LINGO_GENERAL,
        .command = 0x00,
        .param = res_buf,
        .param_size = 0};

    printf("Start to request USB Host mode\n");
    CHECK_ACC_AUTH(trans);

    iap_increment_transid(trans);

    // request to change USB mode
    err = iap_send_command(trans, &req, &res);
    CHECK_ERR(err, "Failed to request USB Host mode\n");
    CHECK_IPOD_ACK((&res), "iDevice declined to switch into USB Host Mode");

    // wait for NotifyUSBMode
    err = iap_recv_command(trans, &res, NULL);
    CHECK_ERR(err, "Failed to receive USB Mode Notification\n");
    if (USB_MODE_HOST != *res.param)
    {
        printf("iDevice has not changed into USB Host mode\n");
        return IAP_ERR;
    }
    printf("Received USB Notification from iDevice that it is in Host mode\n");

    return IAP_OK;
}

int iap_request_wifi_credentials(iap_transport_t *trans)
{
    int err = IAP_OK;
    // acquire some memory to perform the iAP handshake
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

    printf("Start to request WiFi connection credentials\n");
    CHECK_ACC_AUTH(trans);

    iap_increment_transid(trans);
    err = iap_send_command(trans, &req, &res);
    CHECK_ERR(err, "Failed to request WiFi connection credentials\n");
    CHECK_IPOD_ACK((&res), "iDevice has not acknowledged the WiFi connection request\n");

    // wait until the user has confirmed the iDevice prompt
    err = iap_recv_command(trans, &res, NULL);
    CHECK_ERR(err, "Failed to receive the WiFi credentials, the user should acknowledge the prompt on the iDevice");

    iap_cmd_wificonninfo_t *wifi_con = (iap_cmd_wificonninfo_t *)res.param;
    printf("WiFi credentials: \n");
    printf("  - MODE: %s\n", (wifi_con->sectype == WIFI_SEC_WPA2) ? "WPA2" : ((wifi_con->sectype == WIFI_SEC_WPA) ? "WPA" : (wifi_con->sectype == WIFI_SEC_WEP) ? "WEP"
                                                                                                                        : (wifi_con->sectype == WIFI_SEC_NONE)  ? "NONE"
                                                                                                                                                                : "WPA & WPA2 MIX"));
    printf("  - SSID: %s\n", wifi_con->ssid);
    printf("  - PWD:  %s\n", wifi_con->pwd);

    return IAP_OK;
}