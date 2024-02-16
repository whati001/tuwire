#include <stdio.h>
#include <string.h>

#include "iap_cp.h"
#include "iap_cp_defs.h"
#include "iap_core.h"
#include "iap_util.h"

int inline iap_cp_read_device_version(iap_cp_t *cp, uint8_t *val)
{
    return iap_cp_read_reg(cp, val, IAP_CP_REG_DEVICE_ID, IAP_CP_REG_DEVICE_ID_LEN);
}

int inline iap_cp_read_firmware_version(iap_cp_t *cp, uint8_t *val)
{
    return iap_cp_read_reg(cp, val, IAP_CP_REG_FIRMWARE_VERSION, IAP_CP_REG_FIRMWARE_VERSION_LEN);
}

int inline iap_cp_read_major_version(iap_cp_t *cp, uint8_t *val)
{
    return iap_cp_read_reg(cp, val, IAP_CP_REG_MAJOR_VERSION, IAP_CP_REG_MAJOR_VERSION_LEN);
}

int inline iap_cp_read_minor_version(iap_cp_t *cp, uint8_t *val)
{
    return iap_cp_read_reg(cp, val, IAP_CP_REG_MINOR_VERSION, IAP_CP_REG_MINOR_VERSION_LEN);
}

int iap_cp_read_cert_size(iap_cp_t *cp, uint16_t *len)
{
    uint16_t cert_buf = 0;
    // read the size of the entire certificate
    if (IAP_ERR == iap_cp_read_reg(cp, (uint8_t *)&cert_buf, IAP_CP_REG_CERT_SIZE, IAP_CP_REG_CERT_SIZE_LEN))
    {
        printf("Failed to read certificate length information\n");
        return IAP_ERR;
    }

    // compute how many pages we need to load from the CP
    cert_buf = swap_uint16(cert_buf);

    if (cert_buf > IAP_CP_CERTIFICATE_MAX_SIZE)
    {
        printf("Something with the length is incorrect, we would request more data than exists on the CP\n");
        return IAP_ERR;
    }

    *len = cert_buf;

    return IAP_OK;
}

void inline iap_cp_reset_cert_page(iap_cp_t *cp)
{
    cp->cert_page = 0;
}

int inline iap_cp_read_cert_page(iap_cp_t *cp, uint8_t *buf, uint8_t len)
{
    return iap_cp_read_reg(cp, buf, IAP_CP_REG_CERT_DATA_START + cp->cert_page++, len);
}

int iap_cp_sign_challenge(iap_cp_t *cp, uint8_t *chlg_data, uint16_t chlg_size, uint8_t *sig_data, uint16_t *sig_size)
{
    int err = IAP_OK;
    uint8_t chal_ret_code = 0;
    // uint8_t chal_start_flag = IAP_CP_START_NEW_SIG;
    uint16_t chlg_size_cp = swap_uint16(chlg_size);
    printf("Start to sign the challenge: ");
    PRINT_ARRAY_RAW(chlg_data, chlg_size);
    printf("\n");

    // write challenge size and data to CP
    err = iap_cp_write_reg(cp, IAP_CP_REG_CHAL_LEN, (uint8_t *)&chlg_size_cp, IAP_CP_REG_CHAL_LEN_LEN);
    CHECK_ERR(err, "Failed to write the challenge length to the CP\n");

    err = iap_cp_write_reg(cp, IAP_CP_REG_CHAL_DATA, chlg_data, chlg_size);
    CHECK_ERR(err, "Failed to write the challenge data to the CP\n");
    printf("Successfully written challenge data and length to CP\n");

    // activate CP to compute the challenge signature
    err = iap_cp_write_reg(cp, IAP_CP_REG_AUTH_STATUS, &((uint8_t){IAP_CP_START_NEW_SIG}), IAP_CP_REG_AUTH_STATUS_LEN);
    CHECK_ERR(err, "Failed to start signature creation\n");

    // wait some time and read the challenge signature computation status
    err = iap_cp_read_reg(cp, (uint8_t *)&chal_ret_code, IAP_CP_REG_AUTH_STATUS, IAP_CP_REG_AUTH_STATUS_LEN);
    CHECK_ERR(err, "Failed to read the error code from the CP, the challenge signature maybe failed\n");

    printf("chal_ret_code: %d\n", chal_ret_code);
    if (IAP_CP_STATUS_SIG_OK != ((chal_ret_code >> 4) & 0b111))
    {
        printf("Failed to create a valid challenge signature\n");
        return IAP_ERR;
    }
    printf("Valid challenge signature created, let's read it\n");

    // read the signed challenge
    err = iap_cp_read_reg(cp, (uint8_t *)sig_size, IAP_CP_REG_SIG_LEN, IAP_CP_REG_SIG_LEN_LEN);
    CHECK_ERR(err, "Failed to read the length of the computed challenge signature\n");
    *sig_size = swap_uint16(*sig_size);
    printf("Read challenge signature length: 0x%04lx\n", *sig_size);

    err = iap_cp_read_reg(cp, (uint8_t *)sig_data, IAP_CP_REG_SIG_DATA, *sig_size);
    CHECK_ERR(err, "Failed to read the data of the computed challenge signature\n");

    printf("Read the challenge signature data: \n");
    PRINT_ARRAY_RAW(sig_data, *sig_size);

    return IAP_OK;
}