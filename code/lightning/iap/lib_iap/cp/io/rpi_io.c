#include <stdio.h>
#include <string.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>

#include "iap_core.h"
#include "iap_util.h"
#include "iap_cp.h"

#define DEF_IAP_CP_SDA_PIN 4
#define DEF_IAP_CP_SCL_PIN 5
#define DEF_IAP_CP_INST i2c_default

int iap_cp_init(iap_cp_t *cp, iap_cp_config_t *conf)
{
    int err = IAP_OK;
    uint8_t sda_pin = DEF_IAP_CP_SDA_PIN;
    uint8_t scl_pin = DEF_IAP_CP_SCL_PIN;
    uint16_t clock = DEF_IAP_CP_CLOCK;
    uint8_t addr = DEF_IAP_CP_ADDR;
    // TODO: check if this instance is already in use
    i2c_inst_t *inst = DEF_IAP_CP_INST;

    if (NULL != conf)
    {
        sda_pin = conf->sda_pin;
        scl_pin = conf->scl_pin;
        clock = conf->clock;
        addr = conf->addr;
    }

    // init i2c interface
    i2c_init(inst, clock);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    // copy over the config
    cp->config.sda_pin = sda_pin;
    cp->config.scl_pin = scl_pin;
    cp->config.clock = clock;
    cp->config.addr = addr;
    cp->inst = inst;
    iap_cp_reset_cert_page(cp);

    err = iap_cp_read_device_version(cp, &cp->version);
    err |= iap_cp_read_major_version(cp, &cp->major);
    err |= iap_cp_read_minor_version(cp, &cp->minor);
    CHECK_ERR(err, "Failed to read device specific information from the iAP CP\n");
    printf("Configured iAP CP{\n  version: 0x%x,\n  major: 0x%x,\n  minor: 0x%x\n}\n", cp->version, cp->major, cp->minor);

    return IAP_OK;
}

int iap_cp_read_reg(iap_cp_t *cp, uint8_t *buf, uint8_t reg, uint8_t len)
{
    int err = 0;
    if (PICO_ERROR_GENERIC == i2c_write_blocking(cp->inst, cp->config.addr, &reg, 1, true))
    {
        printf("Failed to read register: %d, something went wrong during the register claiming\n", reg);
        return IAP_ERR;
    }
    if (PICO_ERROR_GENERIC == i2c_read_blocking(cp->inst, cp->config.addr, buf, len, false))
    {
        printf("Failed to read register: %d, something went wrong during the value read\n", reg);
        return IAP_ERR;
    }

    // if we have fetched a register, let's check if the CP raised some error
    // to avoid a infinite recursion do not check the error status if we fetch the error status
    if (IAP_CP_REG_ERROR_CODE != reg)
    {
        if (IAP_ERR == iap_cp_read_reg(cp, (uint8_t *)&err, IAP_CP_REG_ERROR_CODE, IAP_CP_REG_ERROR_CODE_LEN))
        {
            printf("Failed to read the error code from the CP, the write maybe failed\n");
            return IAP_ERR;
        }
    }

    if (err != 0)
    {
        printf("Read register failed on CP with err: %d\n", err);
        return IAP_ERR;
    }

    return IAP_OK;
}

int iap_cp_write_reg(iap_cp_t *cp, uint8_t reg, uint8_t *buf, uint8_t len)
{
    int err = 0;
    cp->buf[0] = reg;
    memcpy((void *)(cp->buf + 1), buf, len);
    if (PICO_ERROR_GENERIC == i2c_write_blocking(cp->inst, cp->config.addr, cp->buf, (len + 1), true))
    {
        printf("Failed to write register: %d, something went wrong during the write\n", reg);
        return IAP_ERR;
    }

    // give the chip some time to relax and verify if everything was successfully
    sleep_ms(10);
    if (IAP_ERR == iap_cp_read_reg(cp, (uint8_t *)&err, IAP_CP_REG_ERROR_CODE, IAP_CP_REG_ERROR_CODE_LEN))
    {
        printf("Failed to read the error code from the CP, the write maybe failed\n");
    }

    if (err != 0)
    {
        printf("Write register failed on CP with err: %d\n", err);
        return IAP_ERR;
    }

    return IAP_OK;
}
