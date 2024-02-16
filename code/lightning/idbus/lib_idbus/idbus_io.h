#ifndef IDBUS_IO_H
#define IDBUS_IO_H

#include "idbus_msg.h"

/*
 * idbus_core library physical layer abstraction
 * The idea is to make the physical communication independent from this lib
 * Hence, any phy lib such as idbus_io can be used, it simply needs to
 * implement the following functions.
 *  - int idbus_init();
 *  - int idbus_open_hifive(int *fd, void *param);
 *  - int idbus_open_tristar(int *fd, void *param);
 *  - int idbus_write(int fd, idbus_msg_t *msg);
 *  - int idbus_read(int fd, idbus_msg_t *msg);
 *  - int idbus_reset(int fd);
 *  - int idbus_close(int fd);
 */

/*
 * Open a new IDBUS instance as HiFive chip
 *
 * params:
 *  fd: int* -> on success a positive descriptor will be returned;on error IDBUS_ERR
 *  param: void* -> pointer to the params
 */
int idbus_open_hifive(int *fd, void *param);

/*
 * Open a new IDBUS instance as Tristar chip
 *
 * params:
 *  fd: int* -> on success a positive descriptor will be returned;on error IDBUS_ERR
 *  param: void* -> pointer to the params
 */
int idbus_open_tristar(int *fd, void *param);

/*
 * Write the msg to the previous opened idbus instance
 *
 * params:
 *  fd: int -> idbus descriptor to write to
 *  msg: idbus_msg_t -> message to write
 */
int idbus_write(int fd, idbus_msg_t *msg);

/*
 * Read the msg to the previous opened idbus instance
 * This call blocks until some message is send by the iDevice
 *
 * params:
 *  fd: int -> idbus descriptor to read from
 *  msg: idbus_msg_t -> message buffer to read into
 */
int idbus_read(int fd, idbus_msg_t *msg);

/*
 * Reset the previous opened idbus instance
 *
 * param:
 *  fd: int -> idbus descriptor to reset
 */
int idbus_reset(int fd);

/*
 * Close the previous opened idbus instance
 *
 * param:
 *  fd: int -> idbus descriptor to close
 */
int idbus_close(int fd);

#endif
