#ifndef IDBUS_HIFIVE_H
#define IDBUS_HIFIVE_H

#include <idbus.h>

// define the length of the various HiFive properties
#define LEN_DIGITIAL_ID 6
#define LEN_INTERFACE_SERIAL_NUMBER 6
#define LEN_INTERFACE_MODULE_NUMBER 20
#define LEN_ACCESSORY_SERIAL_NUMBER 20
#define LEN_ACCESSORY_STATE 4

// define the length of the various Tristar properties
#define LEN_SEVEN_FOUR 2
#define LEN_CHARGING_STATE 2
#define LEN_SYSTEM_NOTIFICATION 5

// define the length of the raw data of each IDBUS message
// this length neither includes the header nor the crc
#define LEN_SEVEN_FOUR_DATA 2
#define LEN_SEVEN_FIVE_DATA 6
#define LEN_SEVEN_ZERO_DATA 2
#define LEN_SEVEN_ONE_DATA 0
#define LEN_SEVEN_SIX_DATA 0
#define LEN_SEVEN_SEVEN_DATA 10
#define LEN_SEVEN_EIGHT_DATA 0
#define LEN_SEVEN_NINE_DATA 20
#define LEN_SEVEN_A_DATA 0
#define LEN_SEVEN_B_DATA 20
#define LEN_SEVEN_TWO_DATA 0
#define LEN_SEVEN_THREE_DATA 4
#define LEN_EIGHT_FOUR_DATA 8

/*
 * IDBUS message struct
 * It holds the raw values for the header and crc element.
 * Because the data can be of any size, a pointer is leveraged in conjunction
 * with a data_len variable. Please ensure, that the buffer lifetime preserves
 * the writing process. If the message does not possess a data section,
 * the data_len will be 0 and the data_ptr a NULL pointer.
 */
typedef struct
{
    uint8_t header;
    uint8_t *data_ptr;
    uint8_t data_len;
    uint8_t crc;
} idbus_msg_t;

/*
 * Properties for the HiFive chip
 */
typedef struct
{
    uint8_t data[LEN_DIGITIAL_ID];
} digital_id_t;

typedef struct
{
    uint8_t data[LEN_INTERFACE_SERIAL_NUMBER];
} interface_serial_number_t;

typedef struct
{
    uint8_t data[LEN_INTERFACE_MODULE_NUMBER];
} interface_module_number_t;

typedef struct
{
    uint8_t data[LEN_ACCESSORY_SERIAL_NUMBER];
} accessory_serial_number_t;

// we do not know yet what this message is for
// it signals some kind of module state change, but which one???
typedef struct
{
    uint8_t data[LEN_ACCESSORY_STATE];
    // first byte seems to give information if we have already received a 0x70 0x80 -> charing message
} accessory_state_t;
// so let's provide some default value which should work out-of-the-box
#define DEF_ACCESSORY_STATE_MSG \
    {                           \
        0x00, 0x00, 0xC0, 0x00  \
    }

/*
 * Struct holding all the variable configuration of an HiFive chip
 */
typedef struct
{
    digital_id_t digital_id;
    uint8_t vendor_id;
    uint8_t product_id;
    uint8_t revision;
    uint8_t flags;
    interface_serial_number_t interface_serial_number;
    interface_module_number_t interface_module_number;
    accessory_serial_number_t accessory_serial_number;
    accessory_state_t accessory_state;
} idbus_hifive_info_t;

/*
 * Struct holding the prepared raw IDBUS message as a byte array
 * The data is populated based on a idbus_hifive_info_t struct
 * Please use the function idbus_init_hifive to prepare such an array
 */
typedef struct
{   
    // TODO: use names instead of the header value (if known)
    uint8_t seven_five_data[LEN_SEVEN_FIVE_DATA];
    idbus_msg_t seven_five;

    // TODO: find a better logic, to trigger the
    //          power handshake for charging
    uint8_t seven_one_count;
    uint8_t seven_one_data[LEN_SEVEN_ONE_DATA];
    idbus_msg_t seven_one;

    uint8_t seven_seven_data[LEN_SEVEN_SEVEN_DATA];
    idbus_msg_t seven_seven;

    uint8_t seven_nine_data[LEN_SEVEN_NINE_DATA];
    idbus_msg_t seven_nine;

    uint8_t seven_b_data[LEN_SEVEN_B_DATA];
    idbus_msg_t seven_b;

    uint8_t seven_three_data[LEN_SEVEN_THREE_DATA];
    idbus_msg_t seven_three;

    uint8_t seven_eight_count;
} idbus_hifive_t;

/*
 * Properties for the Tristar chip
 */
// we do not know yet what this message is for exactly means
// it requests the digital_id of the HiFive chip.
typedef struct
{
    uint8_t data[LEN_SEVEN_FOUR];
} seven_four_msg_t;
// so let's provide some default value which should work out-of-the-box
#define DEF_SEVEN_FOUR_MSG \
    {                      \
        0x00, 0x02         \
    }

typedef struct
{
    uint8_t data[LEN_CHARGING_STATE];
} charging_state_msg_t;

#define DEF_CHARGING_DEACTIVATED_MSG \
    {                                \
        0x00, 0x00                   \
    }
#define DEF_CHARGING_ACTIVE_MSG \
    {                           \
        0x00, 0x80              \
    }

// we do not know how the summary is computed
typedef struct
{
    uint8_t data[LEN_SYSTEM_NOTIFICATION];
} ios_version_msg_t, model_number_msg_t;

/*
 * Struct holding all the variable configuration of an Tristar chip
 */
typedef struct
{
    seven_four_msg_t seven_four_msg;
    charging_state_msg_t charing_active;
    charging_state_msg_t charing_deactivated;
    ios_version_msg_t ios_version;
    model_number_msg_t model_number;

} idbus_tristar_info_t;

/*
 * Struct holding the prepared raw IDBUS message as a byte array
 * The data is populated based on a idbus_tristar_info_t struct
 * Please use the function idbus_init_tristar to prepare such an array
 */
typedef struct
{
    uint8_t seven_four_data[LEN_SEVEN_FOUR_DATA];
    idbus_msg_t seven_four;

    uint8_t seven_zero_data[LEN_SEVEN_ZERO_DATA];
    idbus_msg_t seven_zero;

    uint8_t seven_six_data[LEN_SEVEN_SIX_DATA];
    idbus_msg_t seven_six;

    uint8_t seven_eight_data[LEN_SEVEN_EIGHT_DATA];
    idbus_msg_t seven_eight;

    uint8_t seven_a_data[LEN_SEVEN_A_DATA];
    idbus_msg_t seven_a;

    uint8_t seven_two_data[LEN_SEVEN_TWO_DATA];
    idbus_msg_t seven_two;

    uint8_t eight_four_data[LEN_EIGHT_FOUR_DATA];
    idbus_msg_t eight_four;

    uint8_t eight_four2_data[LEN_EIGHT_FOUR_DATA];
    idbus_msg_t eight_four2;
} idbus_tristar_t;

// TODO: add some logic to record the entire handshake session
// typedef struct
// {
// } idbus_handshake_t;
#endif