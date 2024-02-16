#ifndef UTIL_H
#define UTIL_H

/*
 * Very basic error handling
 */
#define OK 0
#define ERR 1

/*
 * Small assert function to simply stop
 */
#define ASSERT             \
    do                     \
    {                      \
        while (1)          \
        {                  \
            sleep_ms(100); \
        }                  \
    } while (0)
#endif