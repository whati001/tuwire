#ifndef IAP_CP_H
#define IAP_CP_H

#include <stdint.h>
#include "iap_cp_defs.h"

/*
 * iAP/MFI Coprocessor (CP) for authenticate accessory
 * against an iDevice
 */

/*
 * CP configuration struct
 * The CP must be connected via I2C
 * This struct allows you do define the
 * address, sda, scl and clock speed for the I2C connection
 */
typedef struct
{
    uint8_t sda_pin;
    uint8_t scl_pin;
    uint16_t clock;
    uint8_t addr;
    // void* inst; // TODO: add support for multiple instances
} iap_cp_config_t;

/*
 * CP instance struct
 * The current version does not allow to create multiple
 * CP instances simultaneously. The user can create multiple
 * instances, however, they will use the same I2C interface
 * Not sure if this works properly!!!!
 *
 * TODO: add proper support for multiple I2C instances
 */
typedef struct
{
    iap_cp_config_t config;
    void *inst;
    uint8_t cert_page;
    uint8_t version;
    uint8_t major;
    uint8_t minor;
    uint8_t buf[IAP_CP_MAX_REG_SIZE + 1];
} iap_cp_t;

/*
 * Function to instantiate a new iap_cp_t object
 *
 * param:
 *  cp: iap_cp_t* -> pointer to new instance
 *  conf: iap_cp_conf_t* -> pointer to configuration for new instance
 */
int iap_cp_init(iap_cp_t *cp, iap_cp_config_t *conf);

/*
 * Function to read the register value into the buffer
 * The function will read up to len bytes from the register
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  buf: uint8_t* -> dst buffer to store value in
 *  reg: uint8_t -> register to read from
 *  len: uint8_t -> length to read from register
 */
int iap_cp_read_reg(iap_cp_t *cp, uint8_t *buf, uint8_t reg, uint8_t len);

/*
 * Function to write to register value from buffer
 * The function will write up to len bytes to the register
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  reg: uint8_t -> register to write to
 *  buf: uint8_t* -> src buffer for writing to reg
 *  len: uint8_t -> length to write to register
 */
int iap_cp_write_reg(iap_cp_t *cp, uint8_t reg, uint8_t *buf, uint8_t len);

/*
 * Retrieve the CP device version information
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  val: uint8_t* -> store device version into
 */
int iap_cp_read_device_version(iap_cp_t *cp, uint8_t *val);

/*
 * Retrieve the CP firmware version information
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  val: uint8_t* -> store firmware version into
 */
int iap_cp_read_firmware_version(iap_cp_t *cp, uint8_t *val);

/*
 * Retrieve the CP major version information
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  val: uint8_t* -> store major version into
 */
int iap_cp_read_major_version(iap_cp_t *cp, uint8_t *val);

/*
 * Retrieve the CP minor version information
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  val: uint8_t* -> store minor version into
 */
int iap_cp_read_minor_version(iap_cp_t *cp, uint8_t *val);

/*
 * Simple function to read the certificate length from the CP
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  len: uint16_t* -> buffer to store length in
 */
int iap_cp_read_cert_size(iap_cp_t *cp, uint16_t *len);

/*
 * Reset the internal certificate page counter
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance for which we should reset the counter
 */
void iap_cp_reset_cert_page(iap_cp_t *cp);

/*
 * Retrieve up to len bytes from the next certificate page from the CP
 * After each call, the internal certificate page counter will get increased
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  buf: uint8_t* -> buffer to store cert data in
 *  len: uint8_t -> length to read from the current page
 */
int iap_cp_read_cert_page(iap_cp_t *cp, uint8_t *buf, uint8_t len);

/*
 * Sign the challenge by leveraging the CP.
 * First, we need to write the challenge data and size to the CP.
 * Second, we trigger the computation and wait some time until the generation finished
 * Finally, we retrieve it back from the CP.
 *
 * param:
 *  cp: iap_cp_t* -> pointer to coprocessor instance
 *  chlg_data: uint8_t* -> pointer to challenge data
 *  chlg_size: uint16_t -> challenge size
 *  sig_data: uint8_t* -> pointer to dst buffer where to store the signature
 *  sig_size: uint16_t* -> final signature size
 */
int iap_cp_sign_challenge(iap_cp_t *cp, uint8_t *chlg_data, uint16_t chlg_size, uint8_t *sig_data, uint16_t *sig_size);

#endif