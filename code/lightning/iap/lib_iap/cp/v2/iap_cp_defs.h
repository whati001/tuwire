#ifndef IAP_CP_REGS_H
#define IAP_CP_REGS_H

/*
 * MFI/iAP Coprocessor defines
 *
 * This file contains all the information
 * to interact with an MFI/iAP 2.0B and 2.0C coprocessor
 */

// taken from datasheet; it only supports up to 50k
#define DEF_IAP_CP_CLOCK 5000
#define DEF_IAP_CP_ADDR 0x11

// define whats the max size of a single reg read;used for write/read buffer
#define IAP_CP_MAX_REG_SIZE 128

// some boundaries
#define IAP_CP_CERTIFICATE_MAX_SIZE (1920)
#define IAP_CP_CHALLENGE_SIZE (20)

// registers
#define IAP_CP_REG_DEVICE_VERSION (0x00)
#define IAP_CP_REG_DEVICE_VERSION_LEN (1)

#define IAP_CP_REG_FIRMWARE_VERSION (0x01)
#define IAP_CP_REG_FIRMWARE_VERSION_LEN (1)

#define IAP_CP_REG_MAJOR_VERSION (0x02)
#define IAP_CP_REG_MAJOR_VERSION_LEN (1)

#define IAP_CP_REG_MINOR_VERSION (0x03)
#define IAP_CP_REG_MINOR_VERSION_LEN (1)

#define IAP_CP_REG_DEVICE_ID (0x04)
#define IAP_CP_REG_DEVICE_ID_LEN (4)

#define IAP_CP_REG_ERROR_CODE (0x05)
#define IAP_CP_REG_ERROR_CODE_LEN (1)

#define IAP_CP_REG_AUTH_STATUS (0x10)
#define IAP_CP_REG_AUTH_STATUS_LEN (1)

#define IAP_CP_REG_SIG_LEN (0x11)
#define IAP_CP_REG_SIG_LEN_LEN (2)
#define IAP_CP_REG_SIG_DATA (0x12)
#define IAP_CP_REG_SIG_DATA_LEN (128)

#define IAP_CP_REG_CHAL_LEN (0x20)
#define IAP_CP_REG_CHAL_LEN_LEN (2)
#define IAP_CP_REG_CHAL_DATA (0x21)
#define IAP_CP_REG_CHAL_DATA_LEN (20)

#define IAP_CP_REG_CERT_SIZE (0x30)
#define IAP_CP_REG_CERT_SIZE_LEN (2)

#define IAP_CP_REG_CERT_DATA_START (0x31)
#define IAP_CP_REG_CERT_DATA_END (0x3F)
#define IAP_CP_REG_CERT_DATA_LEN (128)

#define IAP_CP_REG_SELF_TEST (0x40)
#define IAP_CP_REG_SELF_TEST_LEN (1)

// status which can be retrieved by reading the IAP_CP_REG_AUTH_STATUS
#define IAP_CP_STATUS_ERR (0x00)
#define IAP_CP_STATUS_SIG_OK (0x01)
#define IAP_CP_STATUS_CHALL_OK (0x02)
// read further on page 21

// actions which can be taken by writing to the IAP_CP_REG_AUTH_STATUS
#define IAP_CP_START_NEW_SIG (0x01)
#define IAP_CP_START_NEW_CHAL_GEN (0x02)
// read further on page 22

#endif