#ifndef IAP_IO_USB_H
#define IAP_IO_USB_H

#include "iap_io.h"

// iap_core IAP_USB implementation
int _iap_init_transport_usb(iap_transport_t *trans, iap_transport_type mode, void *param);
int _iap_transfer_out_usb(iap_transport_t *trans, uint8_t res_needed);
int _iap_transfer_in_usb(iap_transport_t *trans, uint8_t res_needed);

#endif