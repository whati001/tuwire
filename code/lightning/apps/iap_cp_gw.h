#ifndef IAP_CP_GW_H
#define IAP_CP_GW_H

#include <stdint.h>

// TuWire custom lingo to gateway iAP-CP commands
#define IAP_LINGO_CPGW 0x99
#define IAP_CMD_CPGW_READ_REG 0x01
#define IAP_CMD_CPGW_WRITE_REG 0x02
#define IAP_CMD_CPGW_RES 0x03

typedef struct __attribute__((__packed__))
{
    uint8_t reg;
    uint8_t length;
    // only valid for write requests
    uint8_t data[0];
} iap_cpgw_req_param_t;

typedef struct __attribute__((__packed__))
{
    uint8_t ret;
    // only valid for read requests
    uint8_t data[0];
} iap_cpgw_res_param_t;

int init_iap_cp_gw(iap_cp_t *cp);

void run_iap_cp_gw(uint8_t *cancel);

#endif