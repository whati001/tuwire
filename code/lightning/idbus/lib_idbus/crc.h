/**********************************************************************
 *
 * Filename:    crc.h
 *
 * Description: A header file describing the various CRC standards.
 * Source: https://barrgroup.com/sites/default/files/crc.zip
 *
 * Notes:
 *
 *
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#ifndef _CRC_H
#define _CRC_H

#include <stdint.h>

#define FALSE 0
#define TRUE !FALSE

/*
 * Select the CRC standard from the list that follows.
 * These values are used by the crc_checksum function.
 */
#define CRC_IDBUS

#if defined(CRC_IDBUS)

typedef unsigned char crc;

#define CRC_NAME "CRC-IDBUS"
#define POLYNOMIAL 0x31
#define INITIAL_REMAINDER 0xFF
#define FINAL_XOR_VALUE 0x00
#define REFLECT_DATA TRUE
#define REFLECT_REMAINDER TRUE

#elif defined(CRC_CCITT)

typedef unsigned short crc;

#define CRC_NAME "CRC-CCITT"
#define POLYNOMIAL 0x1021
#define INITIAL_REMAINDER 0xFFFF
#define FINAL_XOR_VALUE 0x0000
#define REFLECT_DATA FALSE
#define REFLECT_REMAINDER FALSE

#elif defined(CRC16)

typedef unsigned short crc;

#define CRC_NAME "CRC-16"
#define POLYNOMIAL 0x8005
#define INITIAL_REMAINDER 0x0000
#define FINAL_XOR_VALUE 0x0000
#define REFLECT_DATA TRUE
#define REFLECT_REMAINDER TRUE

#elif defined(CRC32)

typedef unsigned long crc;

#define CRC_NAME "CRC-32"
#define POLYNOMIAL 0x04C11DB7
#define INITIAL_REMAINDER 0xFFFFFFFF
#define FINAL_XOR_VALUE 0xFFFFFFFF
#define REFLECT_DATA TRUE
#define REFLECT_REMAINDER TRUE

#else

#error "One of CRC_IDBUS, CRC_CCITT, CRC16, or CRC32 must be #define'd."

#endif

/*
 * Compute a generic CRC checksum. The only variable, which cannot be changed is the size.
 * This value is fixed during the compilation via the crc typedef.
 *
 * params:
 *  data: uint8_t* -> data to compute the CRC checksum for
 *  data_len: uint16_t -> length of data pointer array
 *  polynomial: uint32_t -> polynomial to use for CRC computation
 *  init: crc -> inital value of the CRC value (to emulate hardware computations)
 *  refin: uint8_t -> either one or zero to define if the input value should be reflected
 *  refout: uint8_t -> either one or zero to define if the output value should be reflected
 *  xorout: crc -> value to XOR with the final output value, use 0 if no XOR is needed
 *
 * return:
 *  crc -> computed CRC value
 */
crc crc_checksum_generic(uint8_t data[], uint16_t data_len, uint32_t polynomial, crc init, uint8_t refin, uint8_t refout, crc xorout);

/*
 * Compute a CRC checksum, based on the current activated type above.
 * This function will leveraged the generic under the hood and pass the defines above as arguments
 *
 * params:
 *  data: uint8_t* -> data to compute the CRC checksum for
 *  data_len: uint16_t -> length of data pointer array
 *
 * return:
 *  crc -> computed CRC value
 */
crc crc_checksum(uint8_t data[], uint16_t data_len);

#endif