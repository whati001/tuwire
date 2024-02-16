#ifndef IAP_CORE_H
#define IAP_CORE_H

/*
 * Abstraction layer to communicate with iAP devices
 * such as iDevices (iPhone, iPad, etc.) via UART/USB/(BLE)
 */

#include <stdint.h>

#include "iap_cp.h"
#include "iap_io.h"
#include "iap_defs.h"

/*
 * Enable debug messages in idbus_io
 * If disabled, no message will be printed at all.
 */
#define IAP_CORE_DEBUG 0
#if IAP_CORE_DEBUG == 1
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

/*
 * Define some error variables
 */
#define IAP_OK 0
#define IAP_ERR (-1UL)

/*
 * Very basic transaction id handling implementation
 * This function allows you to generate a new transaction id
 * More general speaking, it increments the current value by one
 *
 * param:
 *  trans: iap_transport_t* -> transport to generate new transID for
 */
void iap_increment_transid(iap_transport_t *trans);

/*
 * Read the current errno value from the transport instance and clear it
 * The errno variable will be set in case of an error.
 * This function will return the errno value directly.
 *
 * param:
 *  trans: iap_transport_t* -> transport to generate new transID for
 */
uint8_t iap_read_errno(iap_transport_t *trans);

/*
 * The communication for the iap_core is configured within the iap_io.h file
 * Please have a look at it for more information about how the PHY communication works
 */

/*
 * Helper function to compute the checksum for the passed iap_msg_t
 * This function should update the iap_msg_t.checksum value
 *
 * param:
 *  msg: iap_msg_t* -> iAP message to compute checksum for
 */
void iap_compute_checksum(iap_msg_t *msg);
#endif