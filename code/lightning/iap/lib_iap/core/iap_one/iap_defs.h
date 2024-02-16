#ifndef IAP_IMPL_H
#define IAP_IMPL_H

#include <stdint.h>
/*
 * iAP1 specific implementations
 */

// Header value
#define IAP_HEADER_MSB 0xFF
#define IAP_HEADER_LSB 0x55
#define IAP_HEADER_SIZE_UART 2
#define IAP_HEADER_SIZE_DEF 2

// Command, lingoId, transId, length sizes
#define IAP_COMMAND_SIZE 1

#define IAP_LINGOID_SIZE 1
#define IAP_TRANSID_SIZE 2
#define IAP_LENGTH_MIN_SIZE 1
#define IAP_LENGTH_MAX_SIZE 3

// if the length is a 3-byte value, this marker value will be in the first byte
#define IAP_LONG_LENGTH_MARKER 0x00

// define the checksum length and proper datatype capable of holding it
#define IAP_CHECKSUM_SIZE 1
typedef uint8_t iap_checksum_t;

/*
 * Definition of a basic iAP command with it's flexible parameters
 * This struct will be wrapped by the iap_msg_t struct before sending it through
 * the transport
 */
typedef struct
{
    uint8_t lingoid;
    uint16_t command;
    uint8_t *param;
    uint32_t param_size;
} iap_command_t;

#define PRINT_IAP_CMD(cmd)                                                                                    \
    printf("\niap_command_t: {\n  lingoid: 0x%x\n  command: 0x%04x\n  params: ", cmd->lingoid, cmd->command); \
    PRINT_ARRAY_RAW(cmd->param, cmd->param_size);                                                             \
    printf("\n");

/*
 * Definition of a basic iAP message
 * This struct will be serialized into a basic uint8_t/byte array and
 * send through the transport to the iDevice
 */
typedef struct
{
    uint8_t header_msb;
    uint8_t header_lsb;
    uint32_t length;
    uint16_t transid;
    iap_command_t *command;
    iap_checksum_t checksum;
} iap_msg_t;

#define IAP_MSG_MIN_SIZE ( \
    IAP_HEADER_SIZE_DEF +  \
    IAP_LINGOID_SIZE +     \
    IAP_COMMAND_SIZE +     \
    IAP_TRANSID_SIZE +     \
    IAP_CHECKSUM_SIZE +    \
    IAP_LENGTH_MIN_SIZE)

// Define all the various lingos
#define IAP_LINGO_GENERAL 0x00
#define IAP_LINGO_MICROPHONE 0x01
#define IAP_LINGO_SIMPLE_REMOTE 0x02
// ... please checkout Table 4-1 iAP1 specs
#define IAP_LINOG_EXT_INTERFACE 0x04
#define IAP_LINGO_USB_HOST_MODE 0x06

// Definition of all the commands
#define IAP_CMD_IPOD_ACK 0x01
#define IAP_CMD_START_IDPS 0x38
#define IAP_CMD_REQMAXTRANSSIZE 0x11
#define IAP_CMD_RESMAXTRANSSIZE 0x12
#define IAP_CMD_SETSFIDTOKENVALUE 0x39;
#define IAP_CMD_ENDIDPS 0x3B
#define IAP_CMD_GETACCAUTHINFO 0x14
#define IAP_CMD_RETACCAUTHINFO 0x15
#define IAP_CMD_RETACCAUTHSIG 0x18
#define IAP_CMD_GETIPODOPTIONLINGO 0x4b
#define IAP_CMD_RETIPODOPTIONLINGO 0x4c
#define IAP_CMD_SETAVAILABLECURRENT 0x54
#define IAP_CMD_SETCHARGINGSTATE 0x56

#define IAP_CMD_NOTIFYUSBMODE 0x04
#define IAP_CMD_SETIPODUBSMODE 0x83
#define IAP_CMD_REQWIFICONNINFO 0x69
#define IAP_CMD_WIFICONNINFO 0x6A

// 3.3.2 Command 0x02: iPodAck
typedef struct __attribute__((__packed__))
{
    uint8_t status;
    uint8_t cmd;
} iap_cmd_ipod_t;

// Table 3-6 iAP command error codes
typedef enum
{
    IPOD_ACK_OK = 0x00,
    IPOD_ACK_ERROR = 0x01
} iap_cmd_ipod_status_t;

// this implementation is not 100% correct, because it does not check if it's really a iPOD_ACK 0x01 cmd
// but in this fashion, we can reuse it for all single error status cmds
#define CHECK_IPOD_ACK(cmd, msg)                                                                   \
    if (((iap_cmd_ipod_t *)cmd->param)->status != IPOD_ACK_OK)                                     \
    {                                                                                              \
        printf("IPadACK error: 0x%x received: %s\n", ((iap_cmd_ipod_t *)cmd->param)->status, msg); \
        return IAP_ERR;                                                                            \
    }

// 3.3.41 Command 0x39: SetFIDTokenValues
// 3.3.42 Command 0x3A: AckFIDTokenValues

// 3.3.44 Command 0x3C: IDPSStatus
typedef struct __attribute__((__packed__))
{
    uint8_t status;
} iap_cmd_idpsstatus_t;

// 3.3.19 Command 0x16: AckAccessoryAuthenticationInfo
typedef struct __attribute__((__packed__))
{
    uint8_t status;
} iap_cmd_ackaccauthinfo;

// 3.3.55 Command 0x4B: GetiPodOptionsForLingo
// 3.3.56 Command 0x4C: RetiPodOptionsForLingo
typedef struct __attribute__((__packed__))
{
    uint8_t lingoid;
    uint64_t optionbitmap;
} iap_cmd_retipodoptionforlingo_t;

// USB Lingo
#define USB_MODE_HOST 0x02

// 3.3.70 Command 0x6A: WiFiConnectionInfo
#define WIFI_SEC_NONE 0x00
#define WIFI_SEC_WEP 0x01
#define WIFI_SEC_WPA 0x02
#define WIFI_SEC_WPA2 0x03
#define WIFI_SEC_MIX 0x04

typedef struct __attribute__((__packed__))
{
    uint8_t status;
    uint8_t sectype;
    uint8_t ssid[32];
    // we cheat here a bit -> the pwd is longer than 1 byte but \0 terminated
    // so [1] will point to the correct address and allows us to acces it nicely
    uint8_t pwd[1];
} iap_cmd_wificonninfo_t;

/*
 * SimpleRemote lingo 0x02
 */
#define IAP_CMD_CONTEXTBTNSTATUS 0x00
#define IAP_CMD_REGISTERDESC 0x0F
#define IAP_CMD_IPODHIDREPORT 0x10
#define IAP_CMD_ACCHIDREPORT 0x11
#define IAP_CMD_UNREGISTERDESC 0x12

typedef struct __attribute__((__packed__))
{
    uint8_t index;
    uint16_t vid;
    uint16_t pid;
    uint8_t country_code;
    uint8_t hid_descriptor[0];
} iap_cmd_registerdesc_t;

typedef struct __attribute__((__packed__))
{
    uint8_t index;
    uint8_t report_type;
    uint8_t report[0];
} iap_cmd_acchidreport_t;
#endif