#ifndef IAP_UTIL_H
#define IAP_UTIL_H

#include <stdio.h>

#define IAP_MIN(a, b) ((a) > (b) ? b : a)
#define IAP_CEIL(A, B) (((A) + (B)-1) / (B))

#ifndef CHECK_ERR
#define CHECK_ERR(err, msg) \
    if (IAP_OK != err)      \
    {                       \
        printf(msg);        \
        return err;         \
    }
#endif

#ifndef CHECK_ERR_CONTINUE
#define CHECK_ERR_CONTINUE(err, msg) \
    if (IAP_OK != err)               \
    {                                \
        printf(msg);                 \
        continue;                    \
    }
#endif

#ifndef CHECK_ERR_NO_RET
#define CHECK_ERR_NO_RET(err, msg) \
    if (IAP_OK != err)             \
    {                              \
        printf(msg);               \
    }
#endif

#ifndef CHECK_ERR_GOTO
#define CHECK_ERR_GOTO(err, label, msg) \
    if (IAP_OK != err)                    \
    {                                     \
        printf(msg);                      \
        goto label;                       \
    }
#endif

#ifndef CHECK_ACC_AUTH
#define CHECK_ACC_AUTH(trans)                                                                             \
    if (0 == (trans)->authenticated)                                                                      \
    {                                                                                                     \
        printf("Accessory is not authenticated yet, please run iap_authenticate_accessory beforehand\n"); \
        return IAP_ERR;                                                                                   \
    }
#endif

#ifndef PRINT_ARRAY_RAW
#define PRINT_ARRAY_RAW(arr, len)                     \
    if (len < 0)                                      \
    {                                                 \
        printf("Failed to print array, len == -1\n"); \
    }                                                 \
    else                                              \
    {                                                 \
        printf("[");                                  \
        for (uint16_t idx = 0; idx < len; idx++)      \
        {                                             \
            if (0 != idx)                             \
            {                                         \
                printf(",");                          \
            }                                         \
            printf("0x%02x", arr[idx]);               \
        }                                             \
        printf("]");                                  \
    }
#endif

#ifndef PRINT_ARRAY
#define PRINT_ARRAY(arr, len)                         \
    if (len < 0)                                      \
    {                                                 \
        printf("Failed to print array, len == -1\n"); \
    }                                                 \
    else                                              \
    {                                                 \
        printf("ArrayWithSize[%d]: ", len);           \
        PRINT_ARRAY_RAW(arr, len);                    \
        printf("\n");                                 \
    }
#endif

/*
 * Helper function to swap the byte order of an uint16_t value
 * This function is useful to for reading the certificate length
 */
uint16_t inline swap_uint16(uint16_t val)
{
    uint8_t low = val & 0x00FF;
    uint8_t high = (val & 0xFF00) >> 8;
    uint16_t swap = (low << 8) | high;

    // printf("Swap uint16: src: %#06lx, dst: %#06lx\n", val, swap);
    return swap;
}

/*
 * Helper function to compute the ceil of a float value
 */
static uint8_t inline iap_ceil(float num)
{
    uint8_t inum = (uint8_t)num;
    if (num == (float)inum)
    {
        return inum;
    }
    return inum + 1;
}

#endif
