
#include "iap_io_usb.h"
#include "iap_core.h"

int _iap_init_transport_usb(iap_transport_t *trans, iap_transport_type mode, void *param)
{
    return IAP_ERR;
}

int _iap_transfer_out_usb(iap_transport_t *trans, uint8_t res_needed)
{
    return IAP_ERR;
}

int _iap_transfer_in_usb(iap_transport_t *trans, uint8_t res_needed)
{
    return IAP_ERR;
}

/*
 * This is working iAP USB implementation for the Zephyr RTOS
 * But it was used in a dedicated Zephyr APP instead of in this
 * library, therefore, the code is commended out.
 * Despite, it should work perfectly...
 */
/*
#include <stdio.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <usb_descriptor.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/byteorder.h>

#include "iap_core.h"
#include "../utils.h"

// define the log level for the nero module
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(iap_io_usb);

// locks to signal something is done
K_SEM_DEFINE(iap_int, 1, 1);
K_SEM_DEFINE(iap_in, 1, 1);
K_SEM_DEFINE(iap_out, 1, 1);

// usb interface definition
struct usb_iap_config
{
    struct usb_if_descriptor if0;
    struct usb_ep_descriptor if0_out_ep;
    struct usb_ep_descriptor if0_in_ep;
    struct usb_ep_descriptor if0_int_ep;
} __packed;

// usb interface definition stored to memory
USBD_CLASS_DESCR_DEFINE(primary, 0)
struct usb_iap_config iap_cfg = {
    .if0 = {
        .bLength = sizeof(struct usb_if_descriptor),
        .bDescriptorType = USB_DESC_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 3,
        .bInterfaceClass = IAP_IF_CLASS,
        .bInterfaceSubClass = IAP_IF_SUBCLASS,
        .bInterfaceProtocol = IAP_IF_PROTOCOL,
        .iInterface = USB_IAP_IF_ID,
    },

    .if0_in_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = IAP_IN_EP_ADDR,
        .bmAttributes = USB_DC_EP_BULK,
        .wMaxPacketSize = sys_cpu_to_le16(CONFIG_IAP_BULK_EP_MPS),
        .bInterval = 0x0a,
    },
    .if0_out_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = IAP_OUT_EP_ADDR,
        .bmAttributes = USB_DC_EP_BULK,
        .wMaxPacketSize = sys_cpu_to_le16(CONFIG_IAP_BULK_EP_MPS),
        .bInterval = 0x0a,
    },
    .if0_int_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = IAP_INT_EP_ADDR,
        .bmAttributes = USB_DC_EP_INTERRUPT,
        .wMaxPacketSize = sys_cpu_to_le16(CONFIG_IAP_BULK_EP_MPS),
        .bInterval = 0x0a,
    },
};

// define the callbacks for the endpoints
static struct usb_ep_cfg_data iap_ep_cfg[] = {
    {
        // use usb transfer library callback
        .ep_cb = usb_transfer_ep_callback,
        .ep_addr = IAP_IN_EP_ADDR,
    },
    {
        // use usb transfer library callback
        .ep_cb = usb_transfer_ep_callback,
        .ep_addr = IAP_OUT_EP_ADDR,
    },
    {
        // use usb transfer library callback
        .ep_cb = usb_transfer_ep_callback,
        .ep_addr = IAP_INT_EP_ADDR,
    },
};

static void iap_interface_config(struct usb_desc_header *head,
                                 uint8_t bInterfaceNumber)
{
    ARG_UNUSED(head);
    LOG_INF("iAP interface received request to update interface number to: %d", bInterfaceNumber);
    iap_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

// register the new Nero USB interface
USBD_DEFINE_CFG_DATA(iap_config) = {
    .usb_device_description = NULL,
    .interface_config = iap_interface_config,
    .interface_descriptor = &iap_cfg.if0,
    .cb_usb_status = NULL,
    .interface = {
        // Apple Device seems to send only vendor specific CONTROLs, so a custom handler is only needed for them
        .class_handler = NULL,
        .custom_handler = NULL,
        .vendor_handler = NULL,
    },
    .num_endpoints = ARRAY_SIZE(iap_ep_cfg),
    .endpoint = iap_ep_cfg,
};

int _iap_init_transport_usb(iap_transport_t *trans, iap_transport_type mode, void *param)
{
    if (IAP_USB != mode)
    {
        printf("iap_core: Please call the function _iap_init_transport_usb only with IAP_USB mode\n");
        return IAP_ERR;
    }
    trans->mode = mode,
    trans->active = 1;
    trans->idps_done = 0;
    trans->authenticated = 0;
    trans->transid = 0;
    trans->max_packet_size = 0;

    // we do not need all this stuff here, so let's keep everything else untouched
    return IAP_OK;
}

void _iap_ep_transfer_done(uint8_t ep, int tsize, void *priv)
{
    debug("iap_core: Received iAP transfer callback for ep: 0x%x\n", ep);
    switch (ep)
    {
    case IAP_IN_EP_ADDR:
    {
        k_sem_give(&iap_in);
        break;
    }
    case IAP_OUT_EP_ADDR:
    {
        uint32_t *read_buf_len = (uint32_t *)priv;
        *read_buf_len = tsize;
        k_sem_give(&iap_out);
        break;
    }
    case IAP_INT_EP_ADDR:
    {
        k_sem_give(&iap_int);
        break;
    }

    default:
    {
        printf("iap_core: Callback fired some unknown EP: 0x%x\n", ep);
        ASSERT("");
    }
    }
}

int _iap_transfer_out_usb(iap_transport_t *trans, uint8_t res_needed)
{
    int err = IAP_OK;

    debug("iap_core: Start new outbound iAP transaction\n");
    // take all semaphores
    k_sem_take(&iap_int, K_FOREVER);
    k_sem_take(&iap_in, K_FOREVER);
    if (res_needed)
    {
        k_sem_take(&iap_out, K_FOREVER);
    }

    // place some data to the in ep
    err = usb_transfer(iap_ep_cfg[IAP_IN_EP_IDX].ep_addr, trans->write_buf, trans->write_buf_len, USB_TRANS_WRITE, _iap_ep_transfer_done, NULL);
    CHECK_ERR(err, "Failed to write IN data to IN ep\n");
    if (res_needed)
    {
        err = usb_transfer(iap_ep_cfg[IAP_OUT_EP_IDX].ep_addr, trans->read_buf, IAP_BUFFER_SIZE, USB_TRANS_READ, _iap_ep_transfer_done, &trans->read_buf_len);
        CHECK_ERR(err, "Failed to register transfer on OUT ep\n");
    }
    // notify the iDevice that there is some data
    err = usb_transfer(iap_ep_cfg[IAP_INT_EP_IDX].ep_addr, NULL, 0, USB_TRANS_WRITE, _iap_ep_transfer_done, NULL);
    CHECK_ERR(err, "Failed to signal iDevice\n");

    // wait until all iAP transactions are done
    k_sem_take(&iap_int, K_FOREVER);
    k_sem_give(&iap_int);
    k_sem_take(&iap_in, K_FOREVER);
    k_sem_give(&iap_in);

    if (res_needed)
    {
        k_sem_take(&iap_out, K_FOREVER);
        k_sem_give(&iap_out);
    }

    return IAP_OK;
}

int _iap_transfer_in_usb(iap_transport_t *trans, uint8_t res_needed)
{
    int err = IAP_OK;

    debug("iap_core: Start new inbound iAP transaction\n");

    k_sem_take(&iap_out, K_FOREVER);
    if (res_needed)
    {
        k_sem_take(&iap_int, K_FOREVER);
        k_sem_take(&iap_in, K_FOREVER);
    }

    // read some data
    err = usb_transfer(iap_ep_cfg[IAP_OUT_EP_IDX].ep_addr, trans->read_buf, IAP_BUFFER_SIZE, USB_TRANS_READ, _iap_ep_transfer_done, &trans->read_buf_len);
    CHECK_ERR(err, "Failed to register transfer on OUT ep\n");

    if (res_needed)
    {
        // place some data to the in ep
        err = usb_transfer(iap_ep_cfg[IAP_IN_EP_IDX].ep_addr, trans->write_buf, trans->write_buf_len, USB_TRANS_WRITE, _iap_ep_transfer_done, NULL);
        CHECK_ERR(err, "Failed to write IN data to IN ep\n");
        // notify the iDevice that there is some data
        err = usb_transfer(iap_ep_cfg[IAP_INT_EP_IDX].ep_addr, NULL, 0, USB_TRANS_WRITE, _iap_ep_transfer_done, NULL);
        CHECK_ERR(err, "Failed to signal iDevice\n");
    }

    // wait until all iAP transactions are done

    k_sem_take(&iap_out, K_FOREVER);
    k_sem_give(&iap_out);
    if (res_needed)
    {
        k_sem_take(&iap_int, K_FOREVER);
        k_sem_give(&iap_int);
        k_sem_take(&iap_in, K_FOREVER);
        k_sem_give(&iap_in);
    }

    return IAP_OK;
}
*/