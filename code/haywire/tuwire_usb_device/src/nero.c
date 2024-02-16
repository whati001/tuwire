// Apple Nero USB device driver

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_USB_NERO_DUMP_H264)
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_interface.h>
#if defined(CONFIG_USB_NERO_STORAGE_FLASH)
#include <zephyr/fs/littlefs.h>
#elif defined(CONFIG_USB_NERO_STORAGE_SDCARD)
#include <ff.h>
#else
#error "Unsupported storage device found"
#endif
#endif // CONFIG_USB_NERO_DUMP_H264

#include "nero.h"

LOG_MODULE_REGISTER(usb_nero, LOG_LEVEL_ERR);

#define DISK_THREAD_PRIO -5

struct usb_nero_config
{
    struct usb_if_descriptor if0;
    struct usb_ep_descriptor if0_out_ep;
    struct usb_ep_descriptor if0_in_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary, 0)
struct usb_nero_config nero_cfg = {
    /* Interface descriptor 0 */
    .if0 = {
        .bLength = sizeof(struct usb_if_descriptor),
        .bDescriptorType = USB_DESC_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = NERO_IF_CLASS,
        .bInterfaceSubClass = NERO_IF_SUBCLASS,
        .bInterfaceProtocol = NERO_IF_PROTOCOL,
        .iInterface = 0,
    },

    /* Data Endpoint OUT */
    .if0_out_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = NERO_OUT_EP_ADDR,
        .bmAttributes = USB_DC_EP_BULK,
        .wMaxPacketSize = sys_cpu_to_le16(USB_NERO_BULK_EP_MPS),
        .bInterval = 0x0a,
    },

    /* Data Endpoint IN */
    .if0_in_ep = {
        .bLength = sizeof(struct usb_ep_descriptor),
        .bDescriptorType = USB_DESC_ENDPOINT,
        .bEndpointAddress = NERO_IN_EP_ADDR,
        .bmAttributes = USB_DC_EP_BULK,
        .wMaxPacketSize = sys_cpu_to_le16(USB_NERO_BULK_EP_MPS),
        .bInterval = 0x0a,
    },
};

// usb nero thread related variables
static K_KERNEL_STACK_DEFINE(usb_nero_thread_stack, CONFIG_USB_NERO_STACK_SIZE);
static struct k_thread usb_nero_thread_data;

// usb nero instance
static usb_nero_t usb_nero;
// cheat a bit and use predefined Nero dics, so we do not need to build them by hand]
#define NERO_ASYN_HPD1_DATA_SIZE (150)
uint8_t NERO_ASYN_HPD1_DATA[] = {0x96, 0x00, 0x00, 0x00, 0x74, 0x63, 0x69, 0x64, 0x68, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x13, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x44, 0x69, 0x73, 0x70, 0x6c, 0x61, 0x79, 0x53, 0x69, 0x7a, 0x65, 0x4d, 0x00, 0x00, 0x00, 0x74, 0x63, 0x69, 0x64, 0x23, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x0e, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x48, 0x65, 0x69, 0x67, 0x68, 0x74, 0x0d, 0x00, 0x00, 0x00, 0x76, 0x62, 0x6d, 0x6e, 0x05, 0x00, 0x00, 0x87, 0x44, 0x22, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x0d, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x57, 0x69, 0x64, 0x74, 0x68, 0x0d, 0x00, 0x00, 0x00, 0x76, 0x62, 0x6d, 0x6e, 0x05, 0x00, 0x00, 0xf0, 0x44, 0x26, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x15, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x49, 0x73, 0x4f, 0x76, 0x65, 0x72, 0x73, 0x63, 0x61, 0x6e, 0x6e, 0x65, 0x64, 0x09, 0x00, 0x00, 0x00, 0x76, 0x6c, 0x75, 0x62, 0x01};

#define NERO_ASYN_HPA1_DATA_SIZE (833)
uint8_t NERO_ASYN_HPA1_DATA[] = {0x41, 0x03, 0x00, 0x00, 0x74, 0x63, 0x69, 0x64, 0x6f, 0x01, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x0f, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74, 0x73, 0x58, 0x01, 0x00, 0x00, 0x76, 0x74, 0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x6d, 0x63, 0x70, 0x6c, 0x4c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x6d, 0x63, 0x70, 0x6c, 0x44, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x33, 0x63, 0x61, 0x63, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x88, 0xe5, 0x40, 0x33, 0x63, 0x61, 0x63, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x88, 0xe5, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x88, 0xe5, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdf, 0x40, 0x33, 0x63, 0x61, 0x63, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdf, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdf, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x6d, 0x63, 0x70, 0x6c, 0x44, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x2e, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x15, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x53, 0x63, 0x72, 0x65, 0x65, 0x6e, 0x4c, 0x61, 0x74, 0x65, 0x6e, 0x63, 0x79, 0x11, 0x00, 0x00, 0x00, 0x76, 0x62, 0x6d, 0x6e, 0x06, 0x7b, 0x14, 0xae, 0x47, 0xe1, 0x7a, 0xa4, 0x3f, 0x2b, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x16, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x45, 0x44, 0x49, 0x44, 0x41, 0x43, 0x33, 0x53, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x0d, 0x00, 0x00, 0x00, 0x76, 0x62, 0x6d, 0x6e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x11, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x55, 0x49, 0x44, 0x2c, 0x00, 0x00, 0x00, 0x76, 0x72, 0x74, 0x73, 0x34, 0x43, 0x32, 0x44, 0x36, 0x30, 0x30, 0x42, 0x2d, 0x30, 0x30, 0x30, 0x30, 0x2d, 0x30, 0x30, 0x30, 0x30, 0x2d, 0x33, 0x30, 0x31, 0x37, 0x2d, 0x30, 0x31, 0x30, 0x33, 0x38, 0x30, 0x35, 0x39, 0x33, 0x32, 0x37, 0x38, 0x2c, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x12, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x00, 0x00, 0x00, 0x76, 0x72, 0x74, 0x73, 0x48, 0x44, 0x4d, 0x49, 0x20, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x65, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x21, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x43, 0x68, 0x61, 0x6e, 0x6e, 0x65, 0x6c, 0x4c, 0x61, 0x79, 0x6f, 0x75, 0x74, 0x3c, 0x00, 0x00, 0x00, 0x76, 0x74, 0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x1b, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x42, 0x75, 0x66, 0x66, 0x65, 0x72, 0x41, 0x68, 0x65, 0x61, 0x64, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x76, 0x61, 0x6c, 0x11, 0x00, 0x00, 0x00, 0x76, 0x62, 0x6d, 0x6e, 0x06, 0xe4, 0xa5, 0x9b, 0xc4, 0x20, 0xb0, 0xb2, 0x3f, 0x67, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x23, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x50, 0x72, 0x65, 0x66, 0x65, 0x72, 0x72, 0x65, 0x64, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x43, 0x68, 0x61, 0x6e, 0x6e, 0x65, 0x6c, 0x4c, 0x61, 0x79, 0x6f, 0x75, 0x74, 0x3c, 0x00, 0x00, 0x00, 0x76, 0x74, 0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define NERO_RPLY_AFMT_DATA_SIZE (246)
uint8_t NERO_RPLY_AFMT_DATA[] = {0xf6, 0x00, 0x00, 0x00, 0x74, 0x63, 0x69, 0x64, 0x22, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0xd, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x45, 0x72, 0x72, 0x6f, 0x72, 0xd, 0x00, 0x00, 0x00, 0x76, 0x62, 0x6d, 0x6e, 0x3, 0x00, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x23, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x50, 0x72, 0x65, 0x66, 0x65, 0x72, 0x72, 0x65, 0x64, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x43, 0x68, 0x61, 0x6e, 0x6e, 0x65, 0x6c, 0x4c, 0x61, 0x79, 0x6f, 0x75, 0x74, 0x3c, 0x00, 0x00, 0x00, 0x76, 0x74, 0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2, 0x00, 0x00, 0x00, 0x1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x76, 0x79, 0x65, 0x6b, 0x21, 0x00, 0x00, 0x00, 0x6b, 0x72, 0x74, 0x73, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x41, 0x75, 0x64, 0x69, 0x6f, 0x43, 0x68, 0x61, 0x6e, 0x6e, 0x65, 0x6c, 0x4c, 0x61, 0x79, 0x6f, 0x75, 0x74, 0x3c, 0x00, 0x00, 0x00, 0x76, 0x74, 0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2, 0x00, 0x00, 0x00, 0x1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0};

#define NERO_RPLY_TIME_DATA_SIZE (24)
uint8_t NERO_RPLY_TIME_DATA[] = {0xbb, 0xfb, 0x4c, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0xca, 0x9a, 0x3b, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// usb stack related stuff
static struct k_sem usb_out_sem;
static struct k_sem usb_in_sem;

#define USB_NERO_MAX_BUFFER_SIZE (CONFIG_USB_NERO_BATCH_SIZE)
static uint32_t usb_in_buf_size;
static uint8_t usb_in_buf[USB_NERO_MAX_BUFFER_SIZE + USB_NERO_BULK_EP_MPS];
static uint32_t usb_out_buf_size;
static uint8_t usb_out_buf[USB_NERO_MAX_BUFFER_SIZE + USB_NERO_BULK_EP_MPS];
static int running = 0;

#if defined(CONFIG_USB_NERO_DUMP_H264)
static struct fs_mount_t fs_h264;
static struct fs_file_t h264_file;
static uint64_t h264_file_size;
#if defined(CONFIG_USB_NERO_STORAGE_FLASH)
// littlefs related stuff to store the received H.264 bitstream
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(h264_storage);
#define H264_STORAGE FIXED_PARTITION_ID(storage_partition)
#elif defined(CONFIG_USB_NERO_STORAGE_SDCARD)
static FATFS h264_storage;
static const char *h264_storage_mount_pt = "/SD:";
#else
#error "Invalid storage device failed to init file system data informaiton"
#endif
#endif // CONFIG_USB_NERO_DUMP_H264

/* Forward declaration of the usb endpoint callbacks */
static void usb_nero_bulk_out(uint8_t ep,
                              enum usb_dc_ep_cb_status_code ep_status);
static void usb_nero_bulk_in(uint8_t ep,
                             enum usb_dc_ep_cb_status_code ep_status);

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data usb_nero_ep_data[] = {
    {.ep_cb = usb_nero_bulk_out,
     .ep_addr = NERO_OUT_EP_ADDR},
    {.ep_cb = usb_nero_bulk_in,
     .ep_addr = NERO_IN_EP_ADDR}};

static void usb_nero_bulk_out(uint8_t ep,
                              enum usb_dc_ep_cb_status_code ep_status)
{
    int ret = NERO_OK;
    uint32_t data_available;
    ret = usb_ep_read_wait(ep, usb_out_buf + usb_out_buf_size, USB_NERO_BULK_EP_MPS, &data_available);
    usb_out_buf_size += data_available;

    // LOG_DBG("Received some bulk out data from iDevice with length: %d, out_buffer_size: %d", data_available, usb_out_buf_size);

    // check if we need to stall the ep
    if (usb_out_buf_size >= USB_NERO_MAX_BUFFER_SIZE)
    {
        // LOG_DBG("Received more data than we can store in OUT buffer, let's send no NAK until we have parsed the already received data");
        k_sem_give(&usb_out_sem);
        // LOG_DBG("Released log for starting parsing the received payload");
        return;
    }

    // check if this is a ZLP
    if (data_available < USB_NERO_BULK_EP_MPS)
    {
        // LOG_INF("Received a ZLP, let's release the log and start parsing the received payload");
        k_sem_give(&usb_out_sem);
        return;
    }

    // clear the NAK to receive more data from the USB host
    ret = usb_ep_read_continue(ep);
    // LOG_DBG("We have neither run into the buffer boundary nor received a ZLP packet, clear NAK with ret: %d", ret);
}

static void usb_nero_bulk_in(uint8_t ep,
                             enum usb_dc_ep_cb_status_code ep_status)
{
    LOG_DBG("iDevice has pulled the data from the IN EP, release lock");
    k_sem_give(&usb_in_sem);
}

static void usb_nero_status_cb(struct usb_cfg_data *cfg,
                               enum usb_dc_status_code status,
                               const uint8_t *param)
{
    ARG_UNUSED(param);
    ARG_UNUSED(cfg);

    /* Check the USB status and do needed action if required */
    switch (status)
    {
    case USB_DC_ERROR:
        LOG_DBG("USB device error");
        break;
    case USB_DC_RESET:
        LOG_DBG("USB device reset detected");
        break;
    case USB_DC_CONNECTED:
        LOG_DBG("USB device connected");
        break;
    case USB_DC_CONFIGURED:
        LOG_DBG("USB device configured");
        break;
    case USB_DC_DISCONNECTED:
        LOG_DBG("USB device disconnected");
        break;
    case USB_DC_SUSPEND:
        LOG_DBG("USB device suspended");
        break;
    case USB_DC_RESUME:
        LOG_DBG("USB device resumed");
        break;
    case USB_DC_INTERFACE:
        LOG_DBG("USB interface selected");
        break;
    case USB_DC_SOF:
        break;
    case USB_DC_UNKNOWN:
    default:
        LOG_DBG("USB unknown state");
        break;
    }
}

static int nero_vendor_handler(struct usb_setup_packet *setup,
                               int32_t *len, uint8_t **data)
{
    PRINT_USB_CONTROL_MSG(setup);

    // CONTROL for the device send by the host
    if (usb_reqtype_is_to_device(setup))
    {
        if (0x40 == setup->bRequest)
        {
            // I guess we can ignore the values
            LOG_INF("Let's ignore the 0x40 request");
            return 0;
        }
        return -ENOTSUP;
    }

    // CONTROL for the host filled by the device accordantly
    else if (usb_reqtype_is_to_host(setup))
    {
        if (0x52 == setup->bRequest)
        {
            LOG_INF("Haywire does not send a value back, so it's not supported");
            *len = 0;
            return -ENOTSUP;
        }
        else if (0x45 == setup->bRequest)
        {
            LOG_INF("Haywire response a 0x01, so let's do the same");
            *usb_in_buf = 0x01;
            *len = 1;
            *data = usb_in_buf;
            return 0;
        }
        return -ENOTSUP;
    }
    else
    {
        return -ENOTSUP;
    }
}

static void usb_nero_interface_config(struct usb_desc_header *head,
                                      uint8_t bInterfaceNumber)
{
    ARG_UNUSED(head);
    nero_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

/* Configuration of the Nero Driver by sending to the USB Driver */
USBD_DEFINE_CFG_DATA(usb_nero_config) = {
    .usb_device_description = NULL,
    .interface_config = usb_nero_interface_config,
    .interface_descriptor = &nero_cfg.if0,
    .cb_usb_status = usb_nero_status_cb,
    .interface = {
        .class_handler = NULL,
        .custom_handler = NULL,
        .vendor_handler = nero_vendor_handler,
    },
    .num_endpoints = ARRAY_SIZE(usb_nero_ep_data),
    .endpoint = usb_nero_ep_data};

/*
 * ############################################################################
 * The USB Nero driver support to store the received H.264 frames on the flash
 * memory. To do so, it will mount the storage to the /h264 mount point
 */
#if defined(CONFIG_USB_NERO_DUMP_H264)
static int usb_nero_setup_fs()
{
    int ret = NERO_OK;
    char h264_fname[255];

    LOG_INF("Start to mount the filesystem to /h264 for storing the Nero NALUs");
#if defined(CONFIG_USB_NERO_STORAGE_FLASH)
    fs_h264.storage_dev = (void *)H264_STORAGE;
    fs_h264.type = FS_LITTLEFS;
    fs_h264.mnt_point = "/h264";
    fs_h264.fs_data = &h264_storage;
#elif defined(CONFIG_USB_NERO_STORAGE_SDCARD)
    fs_h264.type = FS_FATFS;
    fs_h264.mnt_point = h264_storage_mount_pt;
    fs_h264.fs_data = &h264_storage;
#else
#error "Invalid storage device, failed to init mount point"
#endif

    ret = fs_mount(&fs_h264);
    if (NERO_OK != ret)
    {
        LOG_ERR("Failed to mount the h264 fs for storing the Nero NALUs");
        return ret;
    }
    LOG_INF("Successfully mounted the h264 fs for storing the Nero NALUs");

    LOG_INF("Start to open a new file for storing the Nero NALUs");
    fs_file_t_init(&h264_file);
    snprintf(h264_fname, sizeof(h264_fname), "%s/%s", fs_h264.mnt_point, "IDEV_REC.264");
    LOG_INF("Filename: %s", h264_fname);
    ret = fs_open(&h264_file, h264_fname, FS_O_CREATE | FS_O_RDWR);
    ret |= fs_seek(&h264_file, 0, FS_SEEK_SET);
    if (NERO_OK != ret)
    {
        LOG_ERR("Failed to initialize the h264 file for storing the Nero NALUs");
        return ret;
    }
    LOG_INF("Successfully opened a new file to store the Nero NALUs");
    return (ret > 0) ? NERO_OK : ret;
}
#endif
/*
 * ############################################################################
 */

static void nero_usb_is_data_available(uint32_t size)
{
    // check if there is enough data available
    if (usb_out_buf_size >= size)
    {
        return;
    }

    if (size > USB_NERO_MAX_BUFFER_SIZE)
    {
        LOG_WRN("Requested more data to be available as USB OUT ep buffer size, truncated to USB_NERO_MAX_BUFFER_SIZE");
    }

    // else, request more
    LOG_DBG("Request more data from the iDevice by clearing the NAK");
    usb_ep_read_continue(usb_nero_ep_data[NERO_OUT_EP_IDX].ep_addr);
    LOG_INF("Wait until data has arrived by trying to taking the lock");
    k_sem_take(&usb_out_sem, K_FOREVER);
}

static void nero_usb_send_data()
{
    int ret = NERO_OK;

    uint32_t bytes = 0;
    k_sem_take(&usb_in_sem, K_FOREVER);
    ret = usb_write(usb_nero_ep_data[NERO_IN_EP_IDX].ep_addr, usb_in_buf, usb_in_buf_size, &bytes);
    if (NERO_OK != ret)
    {
        LOG_ERR("Failed to transmit data to the USB host");
        ASSERT;
    }
    LOG_DBG("Written %d bytes to Nero in ep", bytes);
    // Wait until the host has pulled the data from the ep
    k_sem_take(&usb_in_sem, K_FOREVER);
    k_sem_give(&usb_in_sem);

    // if the packet size is a multiple of the MPS send a ZLP to signal completion
    if (usb_in_buf_size % USB_NERO_BULK_EP_MPS == 0)
    {
        k_sem_take(&usb_in_sem, K_FOREVER);
        ret = usb_write(usb_nero_ep_data[NERO_IN_EP_IDX].ep_addr, NULL, 0, NULL);
        if (NERO_OK != ret)
        {
            LOG_ERR("Failed to transmit a ZLP to the USB host");
            ASSERT;
        }
        LOG_DBG("Written ZLP to Nero in ep to signal end of packet");
        k_sem_take(&usb_in_sem, K_FOREVER);
        k_sem_give(&usb_in_sem);
    }
}

/*
 * ############################################################################
 * Lazy helper function to parse config objects
 */

// add some helper function to parse the nero packages
static uint8_t *nero_find_in_blob(uint8_t *data, uint32_t data_size, uint8_t *key, uint8_t key_size)
{
    LOG_DBG("Start to search for blob index in binary data");
    uint8_t *offset = data;
    while (offset < data + data_size)
    {
        uint8_t i;
        for (i = 0; i < key_size; i++)
        {
            if (key[i] != offset[i])
            {
                goto next;
            }
        }
        LOG_DBG("Found binary blob inside data array");
        return offset;
    next:
        offset += i + 1;
    }
    return NULL;
}

nero_dict_value_t *nero_find_dict_entry(char *keyname, nero_dict_t *dict)
{
    LOG_DBG("Start to verify if we have received a valid configuration dictionary");
    if (DICT_MAGIC != dict->magic)
    {
        LOG_ERR("Received some data which is not a valid dictionary");
        return NULL;
    }
    LOG_DBG("Found a valid dictionary start searching for key: %s", keyname);

    uint8_t *data = (uint8_t *)dict;
    uint8_t *offset = data + sizeof(nero_dict_t);
    nero_dict_entry_t *key = NULL;
    nero_dict_value_t *value = NULL;
    while (offset < (data + dict->size))
    {
        key = (nero_dict_entry_t *)(offset);
        value = (nero_dict_value_t *)(((uint8_t *)&key->key_size) + key->key_size);

        if (0 == strncmp(keyname, key->key_data, strlen(keyname)))
        {
            return value;
        }
        offset += key->size;
    }

    return NULL;
}

static int nero_parse_sps_pps_info(nero_dict_t *cvrp_dict)
{
    char *dict_entry_key = "FormatDescription";
    nero_dict_value_t *cw_buffer = nero_find_dict_entry(dict_entry_key, cvrp_dict);
    LOG_DBG("Found %s object at: %p", dict_entry_key, cw_buffer);
    if (NULL != cw_buffer)
    {
        // INFO: we do some cheating here; instead of parsing the entire CMFormatDescription class
        //  we simply search for a binary needle in the haystack
        //  the needle marks the beginning of the pps/sps block and is shown below
        uint8_t needle_size = 6;
        uint8_t needle[] = {0x6B, 0x78, 0x64, 0x69, 0x69, 0x00}; // kxdii\0
        uint8_t *pps_sps_blob = nero_find_in_blob(cw_buffer->data, cw_buffer->size, needle, needle_size);
        if (pps_sps_blob)
        {
            LOG_DBG("Found PPS/SPS needle at: %p in the haystack", (void *)pps_sps_blob);
            nero_dict_value_t *pss_sps = (nero_dict_value_t *)(pps_sps_blob + needle_size);

            usb_nero.sps_size = (((pss_sps->data[6] & 255) << 8) + (pss_sps->data[7] & 255));
            memcpy(usb_nero.sps_data, pss_sps->data + 8, usb_nero.sps_size);

            usb_nero.pps_size = (((pss_sps->data[usb_nero.sps_size + 9] & 255) << 8) + (pss_sps->data[usb_nero.sps_size + 10] & 255));
            memcpy(usb_nero.pps_data, pss_sps->data + (11 + usb_nero.sps_size), usb_nero.sps_size);
            return NERO_OK;
        }
    }
    return NERO_ERR;
}

static void usb_nero_remove_recv_data(uint32_t size)
{
    LOG_DBG("Remove the first %d bytes from the usb_out_buf", size);
    uint32_t reminding_data = usb_out_buf_size - size;
    LOG_DBG("Original-Size: %d; Reminding size: %d", usb_out_buf_size, reminding_data);
    memmove(usb_out_buf, usb_out_buf + size, reminding_data);
    usb_out_buf_size = reminding_data;
}

static int usb_nero_parse_sdat_header(nero_asyn_feed_t *feed, nero_asyn_feed_sdat_t *res)
{
    int8_t needle_size = 4;
    uint8_t needle[] = {0x74, 0x61, 0x64, 0x73};
    uint8_t *sdat = nero_find_in_blob(feed->cmd.params, 1024, needle, needle_size);
    if (NULL == sdat)
    {
        return NERO_ERR;
    }

    res->start = sdat - 4;
    res->offset = (uint32_t)((uint8_t *)res->start - (uint8_t *)feed);
    res->size = *((uint32_t *)res->start);
    res->data_offset = res->offset + sizeof(uint32_t) * 2;
    res->preamble = sizeof(uint32_t) * 2;
    return NERO_OK;
}

/*
 * ############################################################################
 */

/*
 * ############################################################################
 * Business logic to parse and build the Nero messages
 */
static inline int nero_usb_parse_ping()
{
    nero_ping_t *ping = (nero_ping_t *)usb_out_buf;
    nero_usb_is_data_available(ping->header.size);
    LOG_DBG("The entire PING message has been arrived, start parsing it");
    // no parsing is needed

    return NERO_OK;
}

static inline int nero_usb_build_ping()
{
    nero_ping_t *ping = (nero_ping_t *)usb_in_buf;
    ping->header.size = sizeof(nero_ping_t);                    // compute size
    ping->header.msg = NERO_MSG_PING;                           // static
    ping->header.correlation = NERO_PING_FROM_HOST_CORRELATION; // static

    usb_in_buf_size = ping->header.size;

    return NERO_OK;
}

static inline int nero_usb_parse_cwpa(uint64_t *correlation)
{
    nero_sync_cwpa_t *cwpa = (nero_sync_cwpa_t *)usb_out_buf;
    nero_usb_is_data_available(cwpa->header.size);
    LOG_DBG("The entire SYNC CWPA message has been arrived, start parsing it");
    usb_nero.idevice_audio_clockref = cwpa->param.clockref;
    LOG_INF("Parsed iDevice audio clockref: 0x%llx from SYNC CWPA message", usb_nero.idevice_audio_clockref);

    *correlation = cwpa->param.correlation;

    return NERO_OK;
}

static inline int nero_usb_build_cwpa(uint64_t correlation)
{
    nero_rply_cwpa_t *cwpa = (nero_rply_cwpa_t *)usb_in_buf;
    cwpa->header.size = sizeof(nero_rply_cwpa_t);       // compute size
    cwpa->header.msg = NERO_MSG_RPLY;                   // static
    cwpa->header.correlation = correlation;             // cmd correlation from SYNC
    cwpa->cmd.id = 0x00;                                // static
    cwpa->param.clockref = usb_nero.acc_audio_clockref; // reference, taken from wireshark

    usb_in_buf_size = cwpa->header.size;

    return NERO_OK;
}

static inline int nero_usb_build_hpd1()
{
    nero_asyn_hdp1_t *hpd1 = (nero_asyn_hdp1_t *)usb_in_buf;
    hpd1->header.size = sizeof(nero_asyn_hdp1_t) + NERO_ASYN_HPD1_DATA_SIZE; // compute size
    hpd1->header.msg = NERO_MSG_ASYN;                                        // static
    hpd1->header.correlation = NERO_EMPTY_CORRELATION;                       // static
    hpd1->cmd.id = NERO_CMD_HPD1;                                            // static
    memcpy(hpd1->cmd.params, NERO_ASYN_HPD1_DATA, NERO_ASYN_HPD1_DATA_SIZE); // static

    usb_in_buf_size = hpd1->header.size;

    return NERO_OK;
}

static inline int nero_usb_build_hpa1()
{
    nero_asyn_hda1_t *hpa1 = (nero_asyn_hda1_t *)usb_in_buf;
    hpa1->header.size = sizeof(nero_asyn_hda1_t) + NERO_ASYN_HPA1_DATA_SIZE; // compute size
    hpa1->header.msg = NERO_MSG_ASYN;                                        // static
    hpa1->header.correlation = usb_nero.idevice_audio_clockref;              // device clockref from SYNC[CWPA] msg
    hpa1->cmd.id = NERO_CMD_HPA1;                                            // static
    memcpy(hpa1->cmd.params, NERO_ASYN_HPA1_DATA, NERO_ASYN_HPA1_DATA_SIZE); // static

    usb_in_buf_size = hpa1->header.size;

    return NERO_OK;
}

static inline int nero_usb_parse_afmt(uint64_t *correlation)
{
    nero_sync_afmt_t *afmt = (nero_sync_afmt_t *)usb_out_buf;
    nero_usb_is_data_available(afmt->header.size);
    LOG_DBG("The entire SYNC AFMT message has been arrived, start parsing it");

    *correlation = afmt->param.correlation;

    return NERO_OK;
}

static inline int nero_usb_build_afmt(uint64_t correlation)
{
    // response: prepare the rply message
    nero_rply_afmt_t *afmt = (nero_rply_afmt_t *)usb_in_buf;
    afmt->header.size = sizeof(nero_rply_afmt_t) + NERO_RPLY_AFMT_DATA_SIZE; // compute size
    afmt->header.msg = NERO_MSG_RPLY;                                        // static
    afmt->header.correlation = correlation;                                  // cmd correlation from SYNC
    afmt->cmd.id = 0x00;                                                     // static
    memcpy(afmt->cmd.params, NERO_RPLY_AFMT_DATA, NERO_RPLY_AFMT_DATA_SIZE); // dict data

    usb_in_buf_size = afmt->header.size;

    return NERO_OK;
}

static inline int nero_usb_parse_cvrp(uint64_t *correlation)
{
    nero_sync_cvrp_t *cvrp = (nero_sync_cvrp_t *)usb_out_buf;
    nero_usb_is_data_available(cvrp->header.size);
    LOG_DBG("The entire SYNC CVRP message has been arrived, start parsing it");
    usb_nero.idevice_video_clockref = cvrp->param.clockref;
    LOG_INF("Parsed iDevice video clockref: 0x%llx from SYNC CVRP message", usb_nero.idevice_audio_clockref);

    if (NERO_OK != nero_parse_sps_pps_info((nero_dict_t *)cvrp->param.dict))
    {
        LOG_ERR("Failed to extract SPS/PPS from CVRP packet");
        return NERO_ERR;
    }
    LOG_INF("Found SPS with size: 0x%x and PPS with size: 0x%x in CVRP data", usb_nero.sps_size, usb_nero.pps_size);

    *correlation = cvrp->param.correlation;

#if defined(CONFIG_USB_NERO_DUMP_H264)
    // store SPS and PPS
    int ret = NERO_OK;
    uint8_t nal_preamble[4] = {0x00, 0x00, 0x00, 0x01};
    ret = fs_write(&h264_file, (void *)nal_preamble, 4);
    if (ret < 0)
    {
        LOG_ERR("Failed to write NAL preamble for SPS");
        return NERO_ERR;
    }
    ret = fs_write(&h264_file, (void *)usb_nero.sps_data, usb_nero.sps_size);
    if (ret < 0)
    {
        LOG_ERR("Failed to write SPS NAL unit");
        return NERO_ERR;
    }
    ret = fs_write(&h264_file, (void *)nal_preamble, 4);
    if (ret < 0)
    {
        LOG_ERR("Failed to write NAL preamble for PPS");
        return NERO_ERR;
    }
    ret = fs_write(&h264_file, (void *)usb_nero.pps_data, usb_nero.pps_size);
    if (ret < 0)
    {
        LOG_ERR("Failed to write PPS NAL unit");
        return NERO_ERR;
    }
    h264_file_size += (usb_nero.sps_size + usb_nero.pps_size + 4 * 2);
#endif

    return NERO_OK;
}

static inline int nero_usb_build_cvrp(uint64_t correlation)
{
    nero_rply_cvrp_t *cvrp = (nero_rply_cvrp_t *)usb_in_buf;
    cvrp->header.size = sizeof(nero_rply_cvrp_t);       // compute size
    cvrp->header.msg = NERO_MSG_RPLY;                   // static
    cvrp->header.correlation = correlation;             // cmd correlation from SYNC
    cvrp->cmd.id = 0x00;                                // static
    cvrp->param.clockref = usb_nero.acc_video_clockref; // acc video clockref

    usb_in_buf_size = cvrp->header.size;
    return NERO_OK;
}

static inline int nero_usb_build_need()
{
    nero_asyn_need_t *need = (nero_asyn_need_t *)usb_in_buf;
    need->header.size = sizeof(nero_asyn_need_t);               // compute size
    need->header.msg = NERO_MSG_ASYN;                           // static
    need->header.correlation = usb_nero.idevice_video_clockref; // received in SYNC[CVRP]
    need->cmd.id = NERO_CMD_NEED;                               // static

    usb_in_buf_size = need->header.size;
    return NERO_OK;
}

static inline int nero_usb_parse_sprp()
{
    nero_asyn_sprp_t *sprp = (nero_asyn_sprp_t *)usb_out_buf;
    nero_usb_is_data_available(sprp->header.size);
    LOG_DBG("The entire SPRP message has been arrived, start parsing it");
    // no parsing is needed

    return NERO_OK;
}

static inline int nero_usb_parse_clok(uint64_t *correlation)
{
    nero_sync_clok_t *clok = (nero_sync_clok_t *)usb_out_buf;
    nero_usb_is_data_available(clok->header.size);
    LOG_DBG("The entire CLOK message has been arrived, start parsing it");
    // no parsing is needed

    *correlation = clok->param.correlation;
    return NERO_OK;
}

static inline int nero_usb_build_clok(uint64_t correlation)
{
    nero_rply_clok_t *clok = (nero_rply_clok_t *)usb_in_buf;
    clok->header.size = sizeof(nero_rply_clok_t);      // compute size
    clok->header.msg = NERO_MSG_RPLY;                  // static
    clok->header.correlation = correlation;            // cmd correlation from SYNC
    clok->cmd.id = 0x00;                               // static
    clok->param.clockref = usb_nero.acc_time_clockref; // acc time clockref

    usb_in_buf_size = clok->header.size;
    return NERO_OK;
}

static inline int nero_usb_parse_time(uint64_t *correlation)
{
    nero_sync_time_t *time = (nero_sync_time_t *)usb_out_buf;
    nero_usb_is_data_available(time->header.size);
    LOG_DBG("The entire TIME message has been arrived, start parsing it");
    // no parsing is needed
    // TODO: consider to store the clock ref somewhere

    *correlation = time->param.correlation;
    return NERO_OK;
}

static inline int nero_usb_build_time(uint64_t correlation)
{
    nero_rply_time_t *time = (nero_rply_time_t *)usb_in_buf;
    time->header.size = sizeof(nero_rply_time_t) + NERO_RPLY_TIME_DATA_SIZE; // compute size
    time->header.msg = NERO_MSG_RPLY;                                        // static
    time->header.correlation = correlation;                                  // cmd correlation from SYNC
    time->cmd.id = 0x00;                                                     // static
    memcpy(time->cmd.params, NERO_RPLY_TIME_DATA, NERO_RPLY_TIME_DATA_SIZE); // clock relevant time information

    usb_in_buf_size = time->header.size;
    return NERO_OK;
}

static inline int nero_usb_parse_tbas()
{
    nero_asyn_tbas_t *tbas = (nero_asyn_tbas_t *)usb_out_buf;
    nero_usb_is_data_available(tbas->header.size);
    LOG_DBG("The entire TBAS message has been arrived, start parsing it");
    // no parsing is needed

    return NERO_OK;
}

static inline int nero_usb_parse_srat()
{
    nero_asyn_srat_t *srat = (nero_asyn_srat_t *)usb_out_buf;
    nero_usb_is_data_available(srat->header.size);
    LOG_DBG("The entire SRAT message has been arrived, start parsing it");
    // no parsing is needed

    return NERO_OK;
}

static inline int nero_usb_parse_tjmp()
{
    nero_asyn_tjmp_t *tjmp = (nero_asyn_tjmp_t *)usb_out_buf;
    nero_usb_is_data_available(tjmp->header.size);
    LOG_DBG("The entire TJMP message has been arrived, start parsing it");
    // no parsing is needed

    return NERO_OK;
}

static inline int nero_usb_parse_feed()
{
    // we are now here in this function with the following state:
    //  - buffer_size: 1024
    //  - lock is released:
    nero_asyn_feed_t *feed = (nero_asyn_feed_t *)usb_out_buf;
    uint32_t total_size = feed->header.size;
    LOG_INF("Received FEED has a total size of: %d", total_size);
    // step 1: find the beginning of the NALUs
    //         this is accomplished by simply requesting at least 1024
    //         bytes and search for the "sdat" needle
    //         the four preamble bytes represents the total NALU size
    nero_usb_is_data_available(USB_NERO_MAX_BUFFER_SIZE);

    nero_asyn_feed_sdat_t sdat;
    if (NERO_OK != usb_nero_parse_sdat_header(feed, &sdat))
    {
        LOG_ERR("Failed to find a sdat container in FEED");
        return NERO_ERR;
    }
    total_size -= sdat.offset;
    total_size -= sdat.size;
    usb_nero_remove_recv_data(sdat.data_offset);
    LOG_INF("Found sdat with offset: %d and size: %d", sdat.offset, sdat.size);

    // step 2: loop over all NALUs
    //         the requirement is, that the nero_out_buffer contains the start of the
    //         NALU with the size preamble
    uint32_t parsed_nalus = sdat.preamble;
    while (parsed_nalus < sdat.size)
    {
        // read the NALU size in big-endian
        nero_usb_is_data_available(sizeof(uint32_t) * 2);
        uint32_t *nalu_size_ptr = (uint32_t *)usb_out_buf;
        uint32_t nalu_size = __bswap_32((*nalu_size_ptr)) + sizeof(uint32_t);
        parsed_nalus += nalu_size;
        LOG_INF("Start to parse NALU with size: %d", nalu_size);

        // append the NAL preamble by leveraging the size field
        usb_out_buf[0] = 0x00;
        usb_out_buf[1] = 0x00;
        usb_out_buf[2] = 0x00;
        usb_out_buf[3] = 0x01;

        // paginate the NALU content
        while (1)
        {
            uint32_t page_size = NERO_MIN(nalu_size, usb_out_buf_size);
            nero_usb_is_data_available(page_size);

#if defined(CONFIG_USB_NERO_DUMP_H264)
            // store content to disk

            // EVALUATION: show how long the write to the storage device takes
            // int64_t ticks_start, ticks_end, diff_ms;
            // ticks_start = k_uptime_ticks();
            // ################################################################

            int ret = fs_write(&h264_file, (void *)usb_out_buf, page_size);
            if (ret < 0)
            {
                LOG_ERR("Failed to write NAL unit containing video data");
                ASSERT;
                return NERO_ERR;
            }
            h264_file_size += page_size;

            // EVALUATION: show how long the write to the storage device takes
            // perform some tasks to measure
            // ticks_end = k_uptime_ticks();

            // compute time duration in (ms)
            // diff_ms = k_ticks_to_ms_floor64(ticks_end - ticks_start);
            // printk("Zephyr: elapsed time(ms): %lld for storage size: %d\n", diff_ms, page_size);
            // ################################################################
#endif
            // update variables
            nalu_size -= page_size;
            LOG_DBG("Loaded page with size: %d", page_size);
            if (nalu_size == 0)
            {
                LOG_DBG("Finished to parse NALU with size: %d", nalu_size);
                usb_nero_remove_recv_data(page_size);
                break;
            }
            else
            {
                // request new data, usb_out_buf_size is zero
                usb_out_buf_size -= page_size;
                nero_usb_is_data_available(page_size);
            }
        }
    }
    LOG_INF("Parsed entire sdat container aka NALUs");

    // step 3: drop the rest of the message
    //         we achieve this by fetching the rest of the FEED
    //         and faking the header size to match with the
    //         size of the remaining total_size
    LOG_INF("Toal size of FEED after NALUs: %d", total_size);
    nero_usb_is_data_available(total_size);
    LOG_INF("Fake header to skip the remaining FEED stuff");
    nero_packet_t *fake_msg = (nero_packet_t *)usb_out_buf;
    fake_msg->header.size = total_size;

    return NERO_OK;
}
/*
 * ############################################################################
 */
static void usb_nero_handle_recv_data()
{
    if (0 == usb_out_buf_size)
    {
        // we are done with the parsing, let's clear the NAK from the ep
        LOG_INF("We are done with parsing the entire usb packet, clear NAK");
        usb_ep_read_continue(usb_nero_ep_data[NERO_OUT_EP_IDX].ep_addr);
        return;
    }

    int err = 0;
    nero_usb_is_data_available(sizeof(nero_packet_header_reg_t));
    nero_packet_header_reg_t *nero_header = (nero_packet_header_reg_t *)usb_out_buf;
    if (nero_header->msg != NERO_MSG_PING)
    {
        nero_usb_is_data_available(sizeof(nero_packet_t));
    }
    nero_packet_t *nero_msg = (nero_packet_t *)usb_out_buf;
    LOG_DBG("Start to parse a new Nero message");
    NERO_PRINT_PACKET_DEBUG(nero_msg);
    switch (nero_msg->header.msg)
    {
    // PING from iDevice
    case NERO_MSG_PING:
    {
        LOG_INF("Received a PING event, no parsing needed simply insert the event");
        err = nero_usb_parse_ping();
        CHECK_ERR(err, "Failed to parse PING message");

        LOG_INF("Handle Nero PING event by replying with a PING to the iDevice");
        err = nero_usb_build_ping();
        CHECK_ERR(err, "Failed to built PING response message");
        LOG_INF("Successfully built and prepared PING message");
        nero_usb_send_data();

        break;
    }
    // SYNC from iDevice
    case NERO_MSG_SYNC:
    {
        uint64_t sync_correlation = 0;
        switch (nero_msg->cmd.id)
        {
        // SYNC CWPA -> parse the iDevice clockref
        case NERO_CMD_CWPA:
        {
            LOG_INF("Received a SYNC CWPA message");
            err = nero_usb_parse_cwpa(&sync_correlation);
            CHECK_ERR(err, "Failed to parse SYNC CWPA message");

            // start to send the reply
            LOG_INF("Handle Nero SYNC CWPA event by replying the local TuWire audio clockref 0x%llx", usb_nero.acc_audio_clockref);
            err = nero_usb_build_cwpa(sync_correlation);
            CHECK_ERR(err, "Failed to built RPLY CWPA response message");
            LOG_INF("Successfully built and prepared RPLY CWPA response message");
            nero_usb_send_data();

            // start to reply HPD1
            LOG_INF("Handle Nero ASYN HPD1 event by sending config object to the iDevice");
            err = nero_usb_build_hpd1();
            CHECK_ERR(err, "Failed to built ASYN HPD1 message");
            LOG_INF("Successfully built and prepared ASYN HPD1 message");
            nero_usb_send_data();

            // start to reply the HPA1
            LOG_INF("Handle Nero ASYN HPA1 event by sending config object to the iDevice");
            err = nero_usb_build_hpa1();
            CHECK_ERR(err, "Failed to built ASYN HPA1 message");
            LOG_INF("Successfully built and prepared ASYN HPA1 message");
            nero_usb_send_data();

            break;
        }

        case NERO_CMD_AFMT:
        {
            LOG_INF("Received a SYNC AFMT message");
            err = nero_usb_parse_afmt(&sync_correlation);
            CHECK_ERR(err, "Failed to parse SYNC AFMT message");

            // start to send the reply
            LOG_INF("Handle Nero SYNC AFMT event by replying the proper configuration object");
            err = nero_usb_build_afmt(sync_correlation);
            CHECK_ERR(err, "Failed to built RPLY AFMT response message");
            LOG_INF("Successfully build and prepared RPLY AFMT response message");
            nero_usb_send_data();

            break;
        }

        case NERO_CMD_CVRP:
        {
            LOG_INF("Received a SYNC CVRP message");
            err = nero_usb_parse_cvrp(&sync_correlation);
            CHECK_ERR(err, "Failed to parse SYNC CVRP message");

            // start to send the reply
            LOG_INF("Handle Nero SYNC CVRP event by replying the local TuWire video clockref 0x%llx", usb_nero.acc_video_clockref);
            err = nero_usb_build_cvrp(sync_correlation);
            CHECK_ERR(err, "Failed to built RPLY CVRP response message");
            LOG_INF("Successfully build and prepared RPLY CVRP response message");
            nero_usb_send_data();

            // start to reply first NEED
            LOG_INF("Handle Nero ASYN NEED event by requesting FEED data");
            err = nero_usb_build_need();
            CHECK_ERR(err, "Failed to build ASYN NEED message");
            LOG_INF("Successfully built and prepared ASYN NEED message");
            nero_usb_send_data();

            break;
        }

        case NERO_CMD_CLOK:
        {
            LOG_INF("Received a SYNC CLOK message");
            err = nero_usb_parse_clok(&sync_correlation);
            CHECK_ERR(err, "Failed to parse SYNC CLOK message");

            // start to send the reply
            LOG_INF("Handle Nero SYNC CLOK event by replying the local TuWire time clockref 0x%llx", usb_nero.acc_time_clockref);
            err = nero_usb_build_clok(sync_correlation);
            CHECK_ERR(err, "Failed to built RPLY CLOK response message");
            LOG_INF("Successfully build and prepared RPLY CLOK response message");
            nero_usb_send_data();

            break;
        }

        case NERO_CMD_TIME:
        {
            LOG_INF("Received a SYNC TIME message");
            err = nero_usb_parse_time(&sync_correlation);
            CHECK_ERR(err, "Failed to parse SYNC TIME message");

            // start to send the reply
            LOG_INF("Handle Nero SYNC TIME event by replying some dummy CMTime object");
            err = nero_usb_build_time(sync_correlation);
            CHECK_ERR(err, "Failed to built RPLY TIME response message");
            LOG_INF("Successfully build and prepared RPLY TIME response message");
            nero_usb_send_data();

            break;
        }

        default:
        {
            ASSERT;
            break;
        }
        }
        break;
    }
    // ASYN from iDevice
    case NERO_MSG_ASYN:
    {
        switch (nero_msg->cmd.id)
        {
        case NERO_CMD_SPRP:
        {
            LOG_INF("Received a ASYN SPRP message");
            err = nero_usb_parse_sprp();
            CHECK_ERR(err, "Failed to parse ASYN SPRP message");
            break;
        }

        case NERO_CMD_TBAS:
        {
            LOG_INF("Received a ASYN TBAS message");
            err = nero_usb_parse_tbas();
            CHECK_ERR(err, "Failed to parse ASYN TBAS message");
            break;
        }

        case NERO_CMD_SRAT:
        {
            LOG_INF("Received a ASYN SRAT message");
            err = nero_usb_parse_srat();
            CHECK_ERR(err, "Failed to parse ASYN SRAT message");
            break;
        }

        case NERO_CMD_TJMP:
        {
            LOG_INF("Received a ASYN TJMP message");
            err = nero_usb_parse_tjmp();
            CHECK_ERR(err, "Failed to parse ASYN TJMP message");
            break;
        }

        case NERO_CMD_FEED:
        {
            LOG_INF("Received a ASYN FEED message");
            err = nero_usb_parse_feed();
            CHECK_ERR(err, "Failed to parse ASYN FEED message");

            // start to reply NEED
            LOG_INF("Handle Nero ASYN FEED event by sending NEED");
            err = nero_usb_build_need();
            CHECK_ERR(err, "Failed to build ASYN NEED message");
            LOG_INF("Successfully built and prepared ASYN NEED message");
            nero_usb_send_data();

            break;
        }

        default:
        {
            ASSERT;
            break;
        }
        }
        break;
    }
    // everything else from iDevice
    default:
    {
        LOG_ERR("Received some unknown packet from iDevice");
        NERO_PRINT_PACKET_ERR(nero_msg);
        running = 0;
        return;
    }
    }

    // call the function recursive until we are done
    // reduce the usb_out_buf_size
    usb_nero_remove_recv_data(nero_msg->header.size);
    usb_nero_handle_recv_data();
}

static void usb_nero_thread_main(int arg1, int unused)
{
    ARG_UNUSED(unused);
    ARG_UNUSED(arg1);

#if defined(CONFIG_USB_NERO_DUMP_H264)
    int ret = NERO_OK;
    ret = usb_nero_setup_fs();
    if (NERO_OK != ret)
    {
        LOG_ERR("Failed to initialize file system for storing the Nero NALUs to");
        return;
    }
    h264_file_size = 0;
#endif

    while (running)
    {
        LOG_DBG("Wait for new messages send by the iDevice");
        k_sem_take(&usb_out_sem, K_FOREVER);
        // taking the the usb_out_sem lock implies that we have some data to process
        // this function will iterate over the received data
        //      and push usb_nero_event_t elements into the FIFO
        LOG_INF("Start to parse the current received USB packet in main loop");
        usb_nero_handle_recv_data();
        LOG_INF("Finished to parse current received USB packet in main loop");

#if defined(CONFIG_USB_NERO_DUMP_H264)
        // more than 10MB
        if (h264_file_size >= (0x100000) * CONFIG_USB_NERO_DUMP_SIZE) // 40 Megabyte
        {
            LOG_INF("Received and stored more than %dMB, let's stop the Nero driver", CONFIG_USB_NERO_DUMP_SIZE);
            running = 0;
        }
#endif
    }

#if defined(CONFIG_USB_NERO_DUMP_H264)
    ret = fs_close(&h264_file);
    ret |= fs_unmount(&fs_h264);
    if (NERO_OK != ret)
    {
        LOG_INF("Successfully closed and unmounted the filesystem where we have stored the H264 file");
    }
#endif
    ASSERT;
}

static int nero_driver_init(const struct device *dev)
{
    int ret = NERO_OK;
    ARG_UNUSED(dev);

    LOG_DBG("Start to initialize the needed syncronization primitives");
    ret = k_sem_init(&usb_out_sem, 0, 1);
    ret |= k_sem_init(&usb_in_sem, 1, 1);

    running = 1;
    usb_out_buf_size = 0;
    usb_in_buf_size = 0;
    usb_nero.acc_audio_clockref = NERO_AUDIO_CLOCK_REF;
    usb_nero.acc_video_clockref = NERO_VIDEO_CLOCK_REF;
    usb_nero.acc_time_clockref = NERO_TIME_CLOCK_REF;

    if (NERO_OK != ret)
    {
        LOG_ERR("Failed to initialize the syncronization primitives");
        return ret;
    }

    /* Start a thread to offload disk ops */
    k_thread_create(&usb_nero_thread_data, usb_nero_thread_stack,
                    CONFIG_USB_NERO_STACK_SIZE,
                    (k_thread_entry_t)usb_nero_thread_main, NULL, NULL, NULL,
                    DISK_THREAD_PRIO, 0, K_NO_WAIT);
    k_thread_name_set(&usb_nero_thread_data, "usb_nero");

    return NERO_OK;
}

SYS_INIT(nero_driver_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
