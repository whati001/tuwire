#ifndef IAP_APP_H
#define IAP_APP_H

#include "iap_core.h"
#include "iap_hid.h"

/*
 * Perform accessory authentication again the iDevice
 * This requires a iAP/MFI Coprocessor (CP) connected to the board.
 * The function will return IAP_OK on success and IAP_ERR if any failure occurs
 *
 * param:
 *  trans: iap_transport_t* -> transport instance to use to perform the acc auth process
 *  cp: iap_cp_t* -> iAP/MFI CP to use for the authentication
 */
int iap_authenticate_accessory(iap_transport_t *trans, iap_cp_t *cp);

/*
 * Request USB Host mode, this will turn the iDevice into USB OTG host mode.
 * This requires a valid accessory authentication, so please call
 * iap_authenticate_accessory beforehand.
 * param:
 *  trans: iap_transport_t* -> transport instance to use
 */
int iap_request_usb_host_mode(iap_transport_t *trans);

/*
 * Request to enable charging
 * This function is very useful, in the case the the digital ID
 * from the IDBUS has not forced charging
 * param:
 *  trans: iap_transport_t* -> transport instance to use
 *  TODO: add variable to define max current
 */
int iap_enable_charging(iap_transport_t *trans);

/*
 * Request WiFi connection credentials
 * This requires a valid accessory authentication, so please call
 * iap_authenticate_accessory beforehand.
 * param:
 *  trans: iap_transport_t* -> transport instance to use
 */
int iap_request_wifi_credentials(iap_transport_t *trans);
#endif