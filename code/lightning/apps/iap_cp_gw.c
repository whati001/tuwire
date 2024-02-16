#include <stdio.h>
#include <stdint.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>

#include "iap_io.h"
#include "iap.h"
#include "iap_util.h"
#include "iap_cp.h"
#include "iap_cp_gw.h"
#include "../iap/iap_core/io/rpi_uart.h"

#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 8 // yellow line
#define UART_RX_PIN 9 // green line

static iap_transport_t iap_host = {0};
static iap_cp_t *iap_cp = NULL;

// acquire some memory to perform the iAP handshake
// do not allocate such big arrays on the stack if your RTOS
//  supports threads with fixed stack size
static uint8_t gw_buf[265] = {0};
static iap_cpgw_req_param_t *gw_req_param = (iap_cpgw_req_param_t *)gw_buf;
static iap_cpgw_res_param_t *gw_res_param = (iap_cpgw_res_param_t *)gw_buf;

static iap_command_t gw_cmd = {
    .lingoid = IAP_LINGO_CPGW,
    .command = IAP_CMD_CPGW_READ_REG,
    .param = gw_buf,
    .param_size = 0};
static iap_msg_t gw_msg = {
    .command = &gw_cmd};

int init_iap_cp_gw(iap_cp_t *cp)
{
    // init uart instance
    int err = IAP_OK;

    iap_uart_transfer_conf_t conf = {
        .tx_pin = UART_TX_PIN,
        .rx_pin = UART_RX_PIN,
        .baud_rate = BAUD_RATE,
        .inst = UART_ID,
        .cb_data_in = NULL,
    };
    err = iap_init_transport(&iap_host, IAP_UART, &conf);
    CHECK_ERR(err, "Failed to initiate UART transport for iAP-Host\n");

    iap_cp = cp;
}

void run_iap_cp_gw(uint8_t *cancel)
{
    int err = IAP_OK;
    uint8_t req_reg = 0, req_length = 0;
    uint16_t req_transid = 0;

    while (0 == *cancel)
    {
        printf("Wait for new iAP-CP request send to the gw\n");
        // read new request
        err = iap_recv_msg(&iap_host, &gw_msg, NULL);
        CHECK_ERR_CONTINUE(err, "Failed to receive new iAP-CP request\n");
        PRINT_IAP_CMD(gw_msg.command);
        if (IAP_LINGO_CPGW != gw_cmd.lingoid)
        {
            printf("Invalid LingoId received, iAP-CP gateway will ignore the request\n");
            continue;
        }
        req_reg = gw_req_param->reg;
        req_length = gw_req_param->length;
        req_transid = gw_msg.transid;

        // handle the gateway request
        if (IAP_CMD_CPGW_READ_REG == gw_cmd.command)
        {
            printf("iAP-CP gateway has received a read request for the register: 0x%x with length: %d\n", req_reg, req_length);
            err = iap_cp_read_reg(iap_cp, gw_res_param->data, req_reg, req_length);
            CHECK_ERR_GOTO(err, send_ret, "Failed to read iAP-CP request to cp chip\n");
            gw_cmd.param_size = sizeof(iap_cpgw_res_param_t) + req_length;
            printf("iAP-CP gateway has received the data from the CP, send back\n");
        }
        else if (IAP_CMD_CPGW_WRITE_REG == gw_cmd.command)
        {
            printf("iAP-CP gateway has received a write request for register: 0x%x with length: %d and data: \n", req_reg, req_length);
            err = iap_cp_write_reg(iap_cp, req_reg, gw_req_param->data, req_length);
            CHECK_ERR_GOTO(err, send_ret, "Failed to write iAP-CP request to cp chip\n");
            gw_cmd.param_size = sizeof(iap_cpgw_res_param_t);
            printf("iAP-CP gateway has written the data from the CP, report success\n");
        }
        else
        {
            printf("Invalid Command received, iAP-CP gateway will ignore the request\n");
            err = IAP_ERR;
        }

    send_ret:
        // return the result code
        gw_res_param->ret = err;
        gw_cmd.command = IAP_CMD_CPGW_RES;
        PRINT_IAP_CMD((&gw_cmd));
        err = iap_send_command(&iap_host, &gw_cmd, NULL);
        CHECK_ERR_CONTINUE(err, "Failed to send iAP-CP response\n");
        printf("Finished to handle iAP cp request send to the gw\n");
    }
}