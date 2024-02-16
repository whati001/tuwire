#ifndef IDBUS_UTIL_H
#define IDBUS_UTIL_H

#include <stdlib.h>

/*
 * Macro to print array as hex string
 */
#define PRINT_HEX_ARRAY(arr, len)         \
    do                                    \
    {                                     \
        printf("{");                      \
        for (uint8_t i = 0; i < len; i++) \
        {                                 \
            if (i > 0)                    \
            {                             \
                printf(",");              \
            }                             \
            printf("0x%x", arr[i]);       \
        }                                 \
        printf("}");                      \
    } while (0)

#endif