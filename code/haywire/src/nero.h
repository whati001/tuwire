#ifndef NERO_DEF_H
#define NERO_DEF_H

#include <stdint.h>
#include <zephyr/sys/byteorder.h>

#define NERO_MIN(i, j) (((i) < (j)) ? (i) : (j))
// #define MAX(i, j) (((i) > (j)) ? (i) : (j))

/*
 * Nero protocol USB device specification
 */
// interface
#define NERO_IF_CLASS USB_BCC_VENDOR
#define NERO_IF_SUBCLASS 42
#define NERO_IF_PROTOCOL 255

// endpoints
#define NERO_OUT_EP_ADDR 0x02
#define NERO_OUT_EP_IDX 0
#define NERO_IN_EP_ADDR 0x83
#define NERO_IN_EP_IDX 1

// Define the maximal BULK endpoint message size -> issue for HS
// 	- https://github.com/zephyrproject-rtos/zephyr/issues/54162
// 512 -> USB HS
#define USB_NERO_BULK_EP_MPS 512

#define PRINT_USB_CONTROL_MSG(msg)                                                                                                            \
    printf("%s control request:", (setup->RequestType.type == 0x00) ? "Standard" : ((setup->RequestType.type == 0x01) ? "Class" : "Vendor")); \
    printf(" * Host-2-Device: 0x%x\n", usb_reqtype_is_to_device(msg));                                                                        \
    printf(" * bmRequestType: 0x%x\n", msg->bmRequestType);                                                                                   \
    printf(" * bRequest:      0x%x\n", msg->bRequest);                                                                                        \
    printf(" * wValue:        0x%x\n", msg->wValue);                                                                                          \
    printf(" * wIndex:        0x%x\n", msg->wIndex);                                                                                          \
    printf(" * wLength:       0x%x\n", msg->wLength);

/*
 * Nero protocol message specification
 * https://github.com/danielpaulus/quicktime_video_hack/blob/main/doc/technical_documentation.md
 */

#define DICT_MAGIC __bswap_32(0x74636964)
/*
 * Define the structure of the Nero dict elements
 */
typedef struct __attribute__((__packed__))
{
    uint32_t size;
    uint32_t magic;
    uint8_t data[0];
} nero_dict_t, nero_dict_value_t;

typedef struct __attribute__((__packed__))
{
    uint32_t size;
    uint32_t _keymagic;  // static "KEYV"
    uint32_t key_size;   // lenght of key_size + key_type + key_data
    uint32_t key_type;   // STRK or IDXK
    uint8_t key_data[0]; // actual key value
    // INFO: the value is missing because the key data is variable
} nero_dict_entry_t;

/*
 * Definition of the nero packet headers region
 * This region contains the size, msg type and message correlation
 */
typedef struct __attribute__((__packed__))
{
    uint32_t size;
    uint32_t msg;
    uint64_t correlation;
} nero_packet_header_reg_t;

/*
 * Define the nero packet command region
 * This region contains the command and a pointer
 * to the params data
 */
typedef struct __attribute__((__packed__))
{
    uint32_t id;
    uint8_t params[0];
} nero_packet_command_reg_t;

/*
 * Definition of the nero packet header
 * The header defines the message type and the total packet size
 */
typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
} nero_packet_t;

/*
 * ############################################################################
 * Nero message and cmd definitions
 * ############################################################################
 */
/*
 * Definition of all the various Nero msg values
 * Please ensure to use the proper big/little endian
 */
typedef enum
{
    NERO_MSG_PING = __bswap_32(0x676e6970),
    NERO_MSG_SYNC = __bswap_32(0x636e7973),
    NERO_MSG_ASYN = __bswap_32(0x6e797361),
    NERO_MSG_RPLY = __bswap_32(0x796c7072),
} nero_msg_t;

/*
 * Definition of all the various Nero message values
 * Please ensure to use the proper big/little endian
 */
typedef enum
{
    NERO_CMD_CWPA = __bswap_32(0x61707763),
    NERO_CMD_HPD1 = __bswap_32(0x31647068),
    NERO_CMD_HPA1 = __bswap_32(0x31617068),
    NERO_CMD_AFMT = __bswap_32(0x746d6661),
    NERO_CMD_CVRP = __bswap_32(0x70727663),
    NERO_CMD_NEED = __bswap_32(0x6465656e),
    NERO_CMD_FEED = __bswap_32(0x64656566),
    NERO_CMD_SPRP = __bswap_32(0x70727073),
    NERO_CMD_CLOK = __bswap_32(0x6b6f6c63),
    NERO_CMD_TIME = __bswap_32(0x656d6974),
    NERO_CMD_TBAS = __bswap_32(0x73616274),
    NERO_CMD_SRAT = __bswap_32(0x74617273),
    NERO_CMD_TJMP = __bswap_32(0x706d6a74),
} nero_cmd_t;

// extracted from Haywire <> iDevice capture
// TODO: add some nero dict serializer/deserializer
#define NERO_PING_FROM_HOST_CORRELATION (__bswap_64((uint64_t)0x0000000001000000))
#define NERO_AUDIO_CLOCK_REF (__bswap_64(0x406ab10000000000))
#define NERO_VIDEO_CLOCK_REF (__bswap_64(0x1068b20000000000))
#define NERO_TIME_CLOCK_REF (__bswap_64(0xf04cb20000000000))
#define NERO_EMPTY_CORRELATION 0x01

/*
 * ############################################################################
 * Nero ping messages
 * ############################################################################
 */
typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
} nero_ping_t;

/*
 * ############################################################################
 * Nero sync messages
 * ############################################################################
 */

/*
 * Definition of SYNC CWPA request and response
 *
 * The iDevice send us a SYNC CWPA and informs us about it's clockref reference
 * In addition, the correlation is 0x1, because the correlationID is send in the cmd_correlation field
 *
 * The RPLY CWPA needs to include the sync.cmd_correlation in rply.correlation
 * In addition, the msg = 0x00 and the clockref needs to include the clockref ref of the acc
 */
typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t correlation;
        uint64_t clockref;
    } param;
} nero_sync_cwpa_t;

typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t clockref;
    } param;
} nero_rply_cwpa_t;

/*
 * Definition of SYNC AFMT request and response
 * The iDevice requests information about which audio format the Nero adapter would like to use
 * the request contains a serialized version of the AudioStreamBasicDescription class
 * https://github.com/phracker/MacOSX-SDKs/blob/10dd4868459aed5c4e6a0f8c9db51e20a5677a6b/MacOSX10.9.sdk/System/Library/Frameworks/CoreAudio.framework/Versions/A/Headers/CoreAudioTypes.h#L226
 * The sync_afmt request contains in the correlation id the clockref from the accessory.
 *
 * RPLY[AFMT] -> we let the iDevice know which audio encoding we would like to use
 * {
 *  "Error": uint32_t(0x00),
 *  "PreferredAudioChannelLayout": [0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0],
 *  "DefaultAudioChannelLayout": [0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0],
 * }
 */

typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t correlation;
        struct __attribute__((__packed__))
        {
            uint64_t sample_rate;
            uint32_t format_id;
            uint32_t format_flags;
            uint32_t bytes_per_packet;
            uint32_t frames_per_packet;
            uint32_t bytes_per_frame;
            uint32_t channel_per_frame;
            uint32_t bits_per_channel;
            uint32_t reserved;
        } audio_stream_basic_description;
    } param;
} nero_sync_afmt_t;

typedef nero_packet_t nero_rply_afmt_t;

/*
 * Defintion of the SYNC CVRP request and response
 *
 * The iDevice send us a new clockref (local or so) and a dict with a the following information
 * SYNC[CVPA] data
 * {
 *   "PreparedQueueHighWaterLevel": {
 *      "flags": uint32_t(0x01),
        "value": uint64_t(0x05),
        "timescale": uint32_t(0x1e),
        "epoch": uint64_t(0x00)
        },
    "PreparedQueueLowWaterLevel": {
        "flags": uint32_t(0x01),
        "value": uint64_t(0x03),
        "timescale": uint32_t(0x1e),
        "epoch": uint64_t(0x00),
        },
    "FormatDescription": <CMFormatDescription>
 * }
 *
 */
typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t correlation;
        uint64_t clockref;
        uint8_t dict[0];
        // TODO: add CMFormatDescription definition
    } param;
} nero_sync_cvrp_t;

typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t clockref;
    } param;
} nero_rply_cvrp_t;

/*
 * Definition of SYNC CLOK request and response
 *
 * The iDevice requests some clock instance, which it can later issue SYNC[TIME] message for.
 * The request from the iDevice referes to our video clock id, which we have included in the SYNC[CVRP] message
 *
 * We need to include the cmd_correlation from the nero_sync_clok_t in the nero_rply_clok_t.correlation.
 */

typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t correlation;
    } param;
} nero_sync_clok_t;

typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t clockref;
    } param;
} nero_rply_clok_t;

/*
 * Definition of SYNC TIME request and response
 *
 * We need to include the cmd_correlation from the nero_sync_clok_t in the nero_rply_clok_t.correlation.
 */

typedef struct __attribute__((__packed__))
{
    nero_packet_header_reg_t header;
    nero_packet_command_reg_t cmd;
    struct __attribute__((__packed__))
    {
        uint64_t correlation;
    } param;
} nero_sync_time_t;

typedef nero_packet_t nero_rply_time_t;

/*
 * ############################################################################
 * Nero async messages
 * ############################################################################
 */

/*
 * HPD1 -> we inform the iDevice what we support
 * As we can see, we send it the max DisplaySize and if overscanned is enabled
 * DATA: NERO_ASYN_HPD1_DATA
 *  {
 *      "DisplaySize": {
 *      "Height": 34628, // (nmbv:NSValue[05])
 *      "Width": 61508 // (nmbv:NSValue[05])
 *      },
 *      "IsOverscanned": 1 // (bulv:Boolean)
 *  }
 */
typedef nero_packet_t nero_asyn_hdp1_t;

/*
 * HPA1 -> we inform the iDevice who it is talking to
 *
 * {
 *  "format": [0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x6d, 0x63, 0x70, 0x6c, 0x4c, 0x00, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x6d, 0x63, 0x70, 0x6c, 0x44, 0x00, 0x00, 0x00,  0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,  0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,  0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x33, 0x63, 0x61, 0x63, 0x4c, 0x00, 0x00, 0x00,  0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x80, 0x88, 0xe5, 0x40,  0x33, 0x63, 0x61, 0x63, 0x4c, 0x00, 0x00, 0x00,  0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x80, 0x88, 0xe5, 0x40,  0x00, 0x00, 0x00, 0x00, 0x80, 0x88, 0xe5, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdf, 0x40,  0x33, 0x63, 0x61, 0x63, 0x4c, 0x00, 0x00, 0x00,  0x00, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdf, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdf, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x6d, 0x63, 0x70, 0x6c, 0x44, 0x00, 0x00, 0x00,  0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,  0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,  0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xe7, 0x40, 0x2e],
 *  "ScreenLatency": float64(0x3F, 0xA4, 0x7A, 0xE1, 0x47, 0xAE , 0x14, 0x7B),
 *  "EDIDAC3Support": uint32_t(0x00),
 *  "deviceUID": "4C2D600B-0000-0000-3017-010380593278",
 *  "deviceName": "HDMI Audio",
 *  "DefaultAudioChannelLayout": [ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
 *  "BufferAheadInterval": [0x06, 0xE4, 0xA5, 0x9B, 0xC4, 0x20, 0xB0, 0xB2],
 *  "PreferredAudioChannelLayout", [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
 * }
 */
typedef nero_packet_t nero_asyn_hda1_t;

/*
 * NEED -> we inform the iDevice that we would like to receive some video data
 *         It's important, that we fill the msg
 */
typedef nero_packet_t nero_asyn_need_t;

/*
 * SPRP -> iDevice provides some properties to set
 *         this message contains a configuration dictionary as cmd.param
 */
typedef nero_packet_t nero_asyn_sprp_t;

/*
 * TBAS -> iDevice sends some info
 */
typedef nero_packet_t nero_asyn_tbas_t;

/*
 * SRAT -> iDevice sends some info
 */
typedef nero_packet_t nero_asyn_srat_t;

/*
 * TJMP -> iDevice sends some info
 */
typedef nero_packet_t nero_asyn_tjmp_t;

/*
 * FEED -> Nero packet containing the video frames (NALUs)
 */
typedef nero_packet_t nero_asyn_feed_t;

typedef struct
{
    uint8_t *start;
    uint8_t preamble;
    uint32_t offset;
    uint32_t size;
    uint32_t data_offset;
} nero_asyn_feed_sdat_t;

#define NERO_OK 0
#define NERO_ERR (-1)

/*
 * Struct representing a USB Nero instance
 */
typedef struct
{
    // video/audio clock reference variables
    uint64_t idevice_audio_clockref;
    uint64_t idevice_video_clockref;

    // video/audio/time clock reference variables
    uint64_t acc_audio_clockref;
    uint64_t acc_video_clockref;
    uint64_t acc_time_clockref;

    // storage for the sequence/picture parameter set
    uint8_t sps_data[50];
    uint8_t sps_size;
    uint8_t pps_data[50];
    uint8_t pps_size;
} usb_nero_t;

/*
 * Struct representing a USB Nero event stored in the FIFO
 */

#define ASSERT                     \
    LOG_ERR("ASSERT !!!!!!!!!!!"); \
    while (1)                      \
    {                              \
        k_msleep(1000);            \
    };

#define CHECK_ERR(err, msg) \
    do                      \
    {                       \
        if (NERO_OK != err) \
        {                   \
            LOG_ERR(msg);   \
            ASSERT;         \
        }                   \
    } while (0);

#define NERO_PRINT_PACKET_DEBUG(packet) \
    NERO_PRINT_PACKET_DETAIL(packet, LOG_DBG)

#define NERO_PRINT_PACKET_INFO(packet) \
    NERO_PRINT_PACKET_DETAIL(packet, LOG_INF)

#define NERO_PRINT_PACKET_ERR(packet) \
    NERO_PRINT_PACKET_DETAIL(packet, LOG_ERR)

#define NERO_PRINT_PACKET_DETAIL(packet, log)                                       \
    do                                                                              \
    {                                                                               \
        uint8_t *tmp = NULL;                                                        \
        log("nero: PacketInfo: {");                                                 \
        log("        - size: 0x%x", packet->header.size);                           \
        tmp = (uint8_t *)&packet->header.msg;                                       \
        log("        - msg: %c%c%c%c", *(tmp), *(tmp + 1), *(tmp + 2), *(tmp + 3)); \
        log("        - msg_cor: 0x%llx", packet->header.correlation);               \
        tmp = (uint8_t *)&packet->cmd.id;                                           \
        log("        - cmd: %c%c%c%c", *(tmp), *(tmp + 1), *(tmp + 2), *(tmp + 3)); \
        log("}");                                                                   \
    } while (0);
#endif