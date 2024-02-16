#include "iap_core.h"
#include "iap_cp.h"

int iap_cp_init(iap_cp_t *cp, iap_cp_config_t *conf)
{
    return IAP_ERR;
}

int iap_cp_read_reg(iap_cp_t *cp, uint8_t *buf, uint8_t reg, uint8_t len)
{
    return IAP_ERR;
}

int iap_cp_write_reg(iap_cp_t *cp, uint8_t reg, uint8_t *buf, uint8_t len)
{
    return IAP_ERR;
}

/*
 * This is working iAP CP implementation for the Zephyr RTOS
 * Unfortunately, the used STM32 chip does not allow to use easily the I2C
 * with a clock speed fo 50kHz. Therefore, this CP_IO implementation
 * exploits the iAP UART protocol and uses the raspberry app `iap_cp_gw.c`
 * to proxy the CP read and write calls. 
 * 
 * Furthermore, it was used in a dedicated Zephyr APP instead of in this
 * library, therefore, the code is commended out.
 * Despite, it should work perfectly...
 */
/*
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include "iap_core.h"
#include "iap_io.h"
#include "iap_cp.h"
#include "iap_util.h"

// define the uart device
#define UART_DEVICE_NODE DT_NODELABEL(usart2)

// TuWire custom lingo to gateway iAP-CP commands
#define IAP_LINGO_CPGW 0x99
#define IAP_CMD_CPGW_READ_REG 0x01
#define IAP_CMD_CPGW_WRITE_REG 0x02
#define IAP_CMD_CPGW_RES 0x03

typedef struct
{
    uint8_t reg;
    union
    {
        uint8_t read_len;
        uint8_t write_len;
    };
    // only valid for write requests
    uint8_t data[0];
} iap_cpgw_req_param_t;

typedef struct
{
    uint8_t ret;
    // only valid for read requests
    uint8_t data[0];
} iap_cpgw_res_param_t;

// TODO: change the logic to somehow wrap this trans into the iap_cp_t instance
iap_transport_t trans = {0};

uint8_t req_buf[IAP_CP_MAX_REG_SIZE + 1];
uint8_t res_buf[IAP_CP_MAX_REG_SIZE + 1];
iap_command_t req = {
    .lingoid = IAP_LINGO_CPGW,
    .command = 0x00,
    .param = req_buf,
    .param_size = 0};
iap_command_t res = {
    .lingoid = IAP_LINGO_CPGW,
    .command = 0x00,
    .param = res_buf,
    .param_size = 0};

int iap_cp_init(iap_cp_t *cp, iap_cp_conf_t *conf)
{
    if (cp->active)
    {
        printf("iap_core: iap_core: iAP CP instance already active, skip init call\n");
        return IAP_OK;
    }
    int err = iap_init_transport(&trans, IAP_UART, NULL);
    CHECK_ERR(err, "Failed to initialize new iAP UART transport\n");
    printf("iap_core: iap_core: Successfully opened iap transfer to iap-cp-gw\n");

    cp->active = 1;
    iap_cp_reset_cert_page(cp);

    err = iap_cp_read_device_version(cp, &cp->version);
    err |= iap_cp_read_major_version(cp, &cp->major);
    err |= iap_cp_read_minor_version(cp, &cp->minor);
    CHECK_ERR(err, "Failed to read device specific information from the iAP CP\n");
    printf("iap_core: iap_core: Configured iAP CP{\n  version: 0x%x,\n  major: 0x%x,\n  minor: 0x%x\n}\n", cp->version, cp->major, cp->minor);

    return IAP_OK;
}

int iap_cp_read_reg(iap_cp_t *cp, uint8_t *buf, uint8_t reg, uint8_t len)
{
    // send request
    req.command = IAP_CMD_CPGW_READ_REG;
    req.param_size = sizeof(iap_cpgw_req_param_t);

    iap_cpgw_req_param_t *req_param = (iap_cpgw_req_param_t *)req.param;
    req_param->reg = reg;
    req_param->read_len = len;
    int err = iap_send_command(&trans, &req, &res);
    CHECK_ERR(err, "Failed to send iAP-CP gateway request\n");

    // recv response
    iap_cpgw_res_param_t *res_param = (iap_cpgw_res_param_t *)res.param;
    memcpy(buf, res_param->data, len);
    return res_param->ret;
}

int iap_cp_write_reg(iap_cp_t *cp, uint8_t reg, uint8_t *buf, uint8_t len)
{
    // send request
    req.command = IAP_CMD_CPGW_WRITE_REG;
    req.param_size = sizeof(iap_cpgw_req_param_t) + len;

    iap_cpgw_req_param_t *req_param = (iap_cpgw_req_param_t *)req.param;
    req_param->reg = reg;
    req_param->write_len = len;
    memcpy(req_param->data, buf, len);
    int err = iap_send_command(&trans, &req, &res);
    CHECK_ERR(err, "Failed to send iAP-CP gateway request\n");

    // recv response
    iap_cpgw_res_param_t *res_param = (iap_cpgw_res_param_t *)res.param;
    return res_param->ret;
}
*/