#ifndef IDBUS_H
#define IDBUS_H

/*
 * Abstraction layer to communicate with IDBUS devices
 * such as iDevices (iPhone, iPad, etc.) and Lightning cables
 */

#include <stddef.h>
#include <stdint.h>
#include "idbus_msg.h"

/*
 * Enable debug messages in idbus_io
 * If disabled, no message will be printed at all.
 */
#define IDBUS_CORE_DEBUG 1
#if IDBUS_CORE_DEBUG == 1
#define idbus_debug(...) printf(__VA_ARGS__)
#else
#define idbus_debug(...)
#endif

/*
 * Define some error variables
 */
#define IDBUS_OK 0
#define IDBUS_ERR (-1UL)
#define IDBUS_ERR_TIMEOUT (IDBUS_ERR - 1UL)
#define IDBUS_ERR_IO_LAYER (IDBUS_ERR - 2UL)

/*
 * Initiate the idbus_core library.
 * This function should also take core of initiating the underlying PHY library
 * as well as setting the idbus_core_phy function pointer properly.
 *
 */
int idbus_init();


/*
 * Initiate a new HiFive instance based on the idbus_hifive_info_t struct.
 * This function will initiate all the message and compute the checksum properly.
 *
 * params:
 *  hifive: idbus_hifive_t* -> result struct representing an entire HiFive instance
 *  info: idbus_hifive_info_t* -> parameters used to create the idbus_hifive_t instance
 */
int idbus_init_hifive(idbus_hifive_t *hifive, idbus_hifive_info_t *info);

/*
 * Initiate a new Tristar instance based on the idbus_tristar_info_t struct.
 * This function will initiate all the message and compute the checksum properly.
 *
 * params:
 *  tristar: idbus_tristar_t* -> result struct representing an entire Tristar instance
 *  info: idbus_tristar_info_t* -> parameters used to create the idbus_tristar_t instance
 */
int idbus_init_tristar(idbus_tristar_t *hifive, idbus_tristar_info_t *info);

/*
 * Helper function to perform the IDBUS handshake as HiFive
 * TODO: This function performs the IDBUS handshake and returns onces "complete".
 *       "Complete" in this case means, that we have performed the powerhandshake + received
 *       two times a 0x84 message. Hence, if this function is called against without a valid
 *       P_IN pin (which does not provide +5V), the Tristar will never request the power
 *       handshake and this function will never return!!!!
 *       It would be more efficient to restructure the function such that it runs on a 
 *       separate thread/core and reports when it is "done"
 *
 * params:
 *  hifive: idbus_hifive_t* -> hifive instance to perform handshake for
 *  params: void* -> parameters for performing the handshake, mostly passed to the underlying PHY lib
 */
int idbus_do_handshake_hifive(idbus_hifive_t *hifive, void *params);

/*
 * Helper function to perform the IDBUS handshake as Tristar
 *
 * params:
 *  tristar: idbus_tristar_t* -> tristar instance to perform handshake for
 *  params: void* -> parameters for performing the handshake, mostly passed to the underlying PHY lib
 */
int idbus_do_handshake_tristar(idbus_tristar_t *tristar, void *params);

/*
 * Compute checksum for the idbus messages
 *
 * params:
 *  msg: idbus_msg_t* -> message to compute CRC checksum for
 */
void idbus_compute_checksum(idbus_msg_t *msg);

/*
 * Helper function to print a idbus_io message
 * if the msg is not valid, nothing will be print
 *
 * params:
 *  msg: idbus_msg_t* -> message to print
 */
void idbus_print_msg(idbus_msg_t *msg);

/*
 * Helper function to print the HiFive info struct
 *
 * params:
 *  hifive: idbus_hifive_t* -> hifive instance to print
 */
void idbus_print_hifive(idbus_hifive_t *hifive);

/*
 * Helper function to print the Tristar info struct
 *
 * params:
 *  tristar: idbus_tristar_t* -> tristar instance to print
 */
void idbus_print_tristar(idbus_tristar_t *tristar);

/*
 * Helper function to print the HiFive info struct
 *
 * params:
 *  info: idbus_hifive_info_t* -> info to print
 */
void idbus_print_hifive_info(idbus_hifive_info_t *info);

/*
 * Helper function to print the tristar info struct
 *
 * params:
 *  info: idbus_tristar_info_t* -> info to print
 */
void idbus_print_tristar_info(idbus_tristar_info_t *info);

/*
 * Simple helper function to verify, if the idbus_msg is valid
 *
 * params:
 *  msg: idbus_msg_t -> message to verify if valid
 *
 * return:
 *  IDBUS_IO_OK: msg is valid
 *  IDBUS_IO_ERROR: msg is not valid
 */
int inline __idbus_is_valid_idbus_msg(idbus_msg_t *msg)
{
    return (NULL != msg) ? IDBUS_OK : IDBUS_ERR;
}

#endif