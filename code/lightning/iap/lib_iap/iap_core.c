#include "iap_core.h"
#include "iap_util.h"

void inline iap_increment_transid(iap_transport_t *trans)
{
    trans->transid++;
}

uint8_t inline iap_read_errno(iap_transport_t *trans)
{
    uint8_t iap_errno = trans->iap_errno;
    trans->iap_errno = 0;
    return iap_errno;
}