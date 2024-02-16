/**********************************************************************
 *
 * Filename:    crc.c
 *
 * Description: Slow and fast implementations of the CRC standards.
 * Source: https://barrgroup.com/sites/default/files/crc.zip
 *
 * Notes:       The parameters for each supported CRC standard are
 *				defined in the header file crc.h.  The implementations
 *				here should stand up to further additions to that list.
 *
 *
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include "crc.h"

#if defined(CRC_SLOW) && defined(CRC_FAST)
#error "It's not supported to have CRC_SLOW and CRC_FAST enabled concurrently."
#endif

/*
 * Derive parameters from the standard-specific parameters in crc.h.
 */
#define WIDTH (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))

/*********************************************************************
 *
 * Function:    reflect()
 *
 * Description: Reorder the bits of a binary sequence, by reflecting
 *				them about the middle position.
 *
 * Notes:		No checking is done that data_len <= 32.
 *
 * Returns:		The reflection of the original data.
 *
 *********************************************************************/
unsigned long reflect(unsigned long data, unsigned char data_len)
{
    unsigned long reflection = 0x00000000;
    unsigned char bit;

    /*
     * Reflect the data about the center bit.
     */
    for (bit = 0; bit < data_len; ++bit)
    {
        /*
         * If the LSB bit is set, set the reflection of it.
         */
        if (data & 0x01)
        {
            reflection |= (1 << ((data_len - 1) - bit));
        }

        data = (data >> 1);
    }

    return (reflection);
}

crc crc_checksum_generic(uint8_t data[], uint16_t data_len, uint32_t polynomial, crc init, uint8_t refin, uint8_t refout, crc xorout)
{
    crc remainder = init;
    int byte;
    unsigned char bit;

    /*
     * Perform modulo-2 division, a byte at a time.
     */
    for (byte = 0; byte < data_len; ++byte)
    {
        /*
         * Bring the next byte into the remainder.
         */
        if (TRUE == refin)
        {
            remainder ^= (reflect(data[byte], 8) << (WIDTH - 8));
        }
        else
        {
            remainder ^= (data[byte] << (WIDTH - 8));
        }

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & TOPBIT)
            {
                remainder = (remainder << 1) ^ polynomial;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }

    /*
     * The final remainder is the CRC result.
     */
    if (TRUE == refout)
    {
        return (reflect(remainder, 8) ^ xorout);
    }
    else
    {
        return (remainder ^ xorout);
    }
}

crc crc_checksum(uint8_t data[], uint16_t data_len)
{
    return crc_checksum_generic(data, data_len, POLYNOMIAL, INITIAL_REMAINDER, REFLECT_DATA, REFLECT_REMAINDER, FINAL_XOR_VALUE);
}