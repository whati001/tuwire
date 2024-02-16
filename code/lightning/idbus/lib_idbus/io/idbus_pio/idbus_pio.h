#ifndef IDBUS_PIO_IMPL_H
#define IDBUS_PIO_IMPL_H

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "idbus.h"

/*
 * Return status codes used by the idbus_pio library
 * On success, a status code of zero should be returned
 * On error, a status code < 0  should be returned
 */
#define IDBUS_PIO_OK IDBUS_OK
#define IDBUS_PIO_ERROR (-1U)
#define IDBUS_PIO_NO_FREE_INSTANCE (IDBUS_PIO_ERROR - 1)
#define IDBUS_PIO_NO_FREE_SM (IDBUS_PIO_ERROR - 2)
#define IDBUS_PIO_NO_UNIQUE_PIN (IDBUS_PIO_ERROR - 3)
#define IDBUS_PIO_INVALID_PARAMS (IDBUS_PIO_ERROR - 4)
#define IDBUS_PIO_TIMEOUT (IDBUS_PIO_ERROR - 5)

/*
 * Define the static value to identify that a BREAK message was received
 * This value is only used if the HIFIVE chip is emulated.
 * Furthermore, this value is leveraged, to identify that TRISTAR has
 * finished sending a message.
 */
#define IDBUS_PIO_BREAK_MSG 0xFFFF

/*
 * Define the PIO clock divisor value
 * This divisor value results into a tick time of approximately 0.5us
 */
#define IDBUS_PIO_CLKDIV (125.0 / 2.0)

/*
 * Define the reading timeout
 * The final timeout in us is the multiplication of the counter values
 * (IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_COUNTER, IDBUS_PIO_RX_NEXT_MSG_TIMEOUT_COUNTER)
 * by the poll time (IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_POLL_US).
 *
 * Furthermore, the timeout counter IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_COUNTER defined,
 * how long the library will wait for the next byte of a message.
 * Hence, if the opposite chip is not sending a new byte within
 * (IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_POLL_US * IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_COUNTER),
 * the read_idbus_pio function will return with the content it has received until
 * the timeout has occurred.
 *
 * Conversely, if the opposite chip does not send anything for the time
 * (IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_POLL_US * IDBUS_PIO_RX_NEXT_MSG_TIMEOUT_COUNTER),
 * the read_idbus_pio function will return with an emtpy received message.
 */
#define IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_POLL_US 5
#define IDBUS_PIO_RX_NEXT_BYTE_TIMEOUT_COUNTER 50
#define IDBUS_PIO_RX_NEXT_MSG_TIMEOUT_COUNTER 10000

/*
 * Set how many idbus_pio instances can be initiated.
 * Each idbus_pio instance is uniquely defined by a GPIO pin.
 * Hence, it's not possible to open multiple instances for the same GPIO pin.
 *
 * However, the real number of instances can be less than this value,
 * if there are no more PIO state machines are free to use. In this case
 * the open_idbus_pio function will fail with an error signalling no sm free.
 */
#define IDBUS_PIO_INSTANCE_COUNT 2
#if IDBUS_PIO_INSTANCE_COUNT > 255
#error "The library does not allow to allocate more than 255 idbus_pio instances"
#endif

/*
 * Define which PIO instance should be used by idbus_pio library
 * The library occupies a single PIO instance. If this PIO is also
 * used by some other code running concurrently, the library may break.
 */
#define IDBUS_PIO_PIO_INSTANCE pio0

/*
 * To make the library more robust and maintain a constant timing,
 * the library will allocate a buffer for writing and reading, which
 * holds the header + data + crc value consecutive. This should
 * normally allow to access the data with a constant time, because
 * no memory regions jumps are needed.
 */
#define IDBUS_PIO_DATA_BUFFER_SIZE 30
#define IDBUS_PIO_META_BUFFER_SIZE 2
#define IDBUS_PIO_MSG_BUFFER_SIZE (IDBUS_PIO_DATA_BUFFER_SIZE + IDBUS_PIO_META_BUFFER_SIZE)
#if IDBUS_PIO_DATA_BUFFER_SIZE > 100
#error "Do you really expect a idbus message to exceed 100 bytes? If so, please remove this check"
#endif

/*
 * CHIP enum passed to the open function
 * The library can be either opened as TRISTAR or HIFIVE chip
 * Based on the selected mode, the write and read PIO code changes slightly
 */
enum CHIP
{
    TRISTAR = 1,
    HIFIVE = 2,
};

/*
 * All available modes of the idbus-io library
 * MODE_TX: chip is ready for sending out idbus messages
 * MODE_RX: chip is ready for receiving idbus messages
 * MODE_IDLE: chip can neither send nor receive
 */
enum MODE
{
    MODE_TX,
    MODE_RX,
    MODE_IDLE
};

/*
 * Instance info struct
 * This struct contains all the instance relevant information
 * It should be initiated within the open and cleared within the close function
 * #TODO: add some compiler flags to ensure entire struct (buffer) is loaded into ram
 */
typedef struct
{
    uint8_t active;                             // bit to show if the current instance is active
    PIO pio;                                    // PIO to use for the idbus communication
    int sm;                                     // state machine id of pio
    uint offset;                                // offset of the code, initial PC position
    float clkdiv;                               // prescaler for the PIO
    uint8_t pin;                                // GPIO pin used for the communicatin
    enum CHIP chip;                             // type of chip to emulate
    uint8_t buf_len;                            // read/write buffer length
    uint8_t buf_data[IDBUS_PIO_MSG_BUFFER_SIZE]; // read/write buffer to make lib more stable
} idbus_pio_t;

/*
 * Array of idbus_pio instances, the number of instances is defined
 * at compile time. Each instance needs to possess a unique GPIO pin,
 * therefore, the library does not support to map two instance to the same
 * GPIO pin. In this case, the open_idbus_pio function will fail.
 */
static idbus_pio_t idbus_pio_inst[IDBUS_PIO_INSTANCE_COUNT];

/*
 * Function to open a new idbus_pio file descriptor to emulate the defined CHIP type.
 * For the communication, the specified GPIO pin will be used.
 *
 * params:
 *  fd: int* -> on success a valid idbus_pio file descriptor is returned
 *  chip: enum CHIP -> define idbus chip (HIFIVE/TRISTAR) to emulate
 *  pin: uint8_t -> GPIO pin to use for idbus communication
 *
 * return:
 *  IDBUS_PIO_OK -> opened idbus file descriptor properly
 *  IDBUS_PIO_NO_UNIQUE_PIN -> pin already used by other idbus_pio instance
 *  IDBUS_PIO_NO_FREE_INSTANCE -> exceeded predefined idbus_pio instances
 *  IDBUS_PIO_INVALID_PARAMS -> invalid chip defined
 */
int open_idbus_pio(int *fd, enum CHIP chip, uint8_t pin);

/*
 * Function to write a given idbus message to the opened file descriptor.
 *
 * params:
 *  fd: int -> file descriptor to send idbus_msg to
 *  msg: iddbus_msg_t* -> message to send out on the specified idbus instance
 *
 * return:
 *  IDBUS_PIO_OK -> send out messages successfully
 *  IDBUS_PIO_INVALID_PARAMS -> invalid file descriptor/chip/message received
 *  IDBUS_PIO_NO_FREE_SM -> no free state machine found
 *  IDBUS_PIO_ERROR -> failed to send idbus message to phy layer
 */
int write_idbus_pio(int fd, idbus_msg_t *msg);

/*
 * Function to read a idbus message from the opened file descriptor.
 *
 * params:
 *  fd: int -> file descriptor to send idbus_msg to
 *  msg: iddbus_msg_t* -> message buffer to store received message in
 *
 * return:
 *  IDBUS_PIO_OK -> received messages successfully
 *  IDBUS_PIO_INVALID_PARAMS -> invalid file descriptor/chip/message received
 *  IDBUS_PIO_NO_FREE_SM -> no free state machine found
 *  IDBUS_PIO_TIMEOUT -> run into message timeout
 *  IDBUS_PIO_ERROR -> failed to receive idbus message from phy layer
 */
int read_idbus_pio(int fd, idbus_msg_t *msg);

/*
 * Function to reset the idbus_pio statemachine by sending a WAKE
 * This function is only available, if the HIFIVE chip is emulated
 *
 * params:
 *  fd: int -> file descriptor to reset idbus (send WAKE)
 *
 * return:
 *  IDBUS_PIO_OK -> reset idbus successfully (send out WAKE)
 *  IDBUS_PIO_INVALID_PARAMS -> invalid file descriptor received
 *  IDBUS_PIO_NO_FREE_SM -> no free state machine found
 */
int idbus_reset_pio(int fd);

/*
 * Function to close idbus_pio instance file descriptor
 *
 * params:
 *  fd: int -> file descriptor to send idbus_msg to
 *
 * return:
 *  IDBUS_PIO_OK -> closed instance properly
 *  IDBUS_PIO_INVALID_PARAMS -> invalid file descriptor received
 */
int close_idbus_pio(int fd);
/*
 * Simple helper function to verify, if the given instance is active
 *
 * params:
 *  inst: idbus_pio_t* -> instance pointer to verify if active
 *
 * return:
 *  IDBUS_PIO_OK: instance is active
 *  IDBUS_PIO_ERROR: instance is not active
 */
int inline __idbus_instance_is_active(idbus_pio_t *inst)
{
    return inst->active == 1 ? IDBUS_PIO_OK : IDBUS_PIO_ERROR;
}

/*
 * Simple helper function to verify, if the fd is valid
 *
 * params:
 *  fd: int -> instance id to verify
 *
 * return:
 *  IDBUS_PIO_OK: instance id is valid
 *  IDBUS_PIO_ERROR: instance id is not valid
 */
int inline __idbus_is_valid_fd(int fd)
{
    return (fd >= 0 && fd <= IDBUS_PIO_INSTANCE_COUNT) ? IDBUS_PIO_OK : IDBUS_PIO_ERROR;
}

/*
 * Simple helper function to verify, if the requested chip is valid

 * params:
 *  chip: enum CHIP -> verify chip value
 *
 * return:
 *  IDBUS_PIO_OK: chip is valid
 *  IDBUS_PIO_ERROR: chip is not valid
 */
int inline __idbus_is_valid_chip(enum CHIP chip)
{
    return (chip == TRISTAR || chip == HIFIVE) ? IDBUS_PIO_OK : IDBUS_PIO_ERROR;
}

/*
 * Simple helper function to clear a given idbus_pio_instance

 * params:
 *  idx: uint8_t -> clear instance with this id
 */
void __idbus_clear_instance(uint8_t idx);

/*
 * Simple helper function to verify if the pin is already used by an active instance

 * params:
 *  pin: uint8_t -> verify if the pin is unique and not used yet
 *
 * return:
 *  IDBUS_PIO_OK: pin is unique and not used yet
 *  IDBUS_PIO_ERROR: pin is not unique and already used by another instance
 */
int __idbus_pin_is_used(uint8_t pin);

/*
 * Simple helper function to reverse the bits within a single byte
 * This function will transform the bits [12345678] into [87654321].
 * Code inspired by: https://github.com/stacksmashing/tamarin-firmware
 *
 * params:
 *  b: uint8_t -> byte to transform bits
 *
 * return:
 *  uint8_t -> byte with transformed bits
 */
uint8_t __idbus_reverse_byte(uint8_t b);

/*
 * Buffer to current msg for the specified instance id.
 * The idea is to make the library more robust by maintaining a
 * own buffer of the msg. This also ensures better timing during sending.
 *
 * params:
 *  id: uint8_t -> instance id
 *  msg: idbus_msg_t* -> message to buffer
 */
void __idbus_buffer_msg(uint8_t id, idbus_msg_t *msg);

/*
 * Unbuffer to current msg for the specified fd.
 * The idea is to make the library more robust by maintaining a
 * own instance of the msg, and pushing the value to the user buffer
 * after a successful read
 *
 * params:
 *  id: uint8_t -> instance id
 *  msg: idbus_msg_t* -> target buffer to write msg to
 */
int __idbus_unbuffer_msg(uint8_t id, idbus_msg_t *msg);

/*
 * Helper function to clear the buffer message of the
 * current instance id
 *
 * params:
 *  id: uint8_t -> instance id
 */
void __idbus_clear_buffer_msg(uint8_t id);

/*
 * Reset the PIO instance used by the idbus_pio library
 * This function also resets the mode variable of all
 * affected idbus_pio_t instances
 *
 * params:
 *  id: uint8_t -> instance id
 */
void __idbus_reset_pio(uint8_t id);

/*
 * Prepare the idbus instance pio for sending
 *
 * params:
 *  id: uint8_t -> instance id
 *
 * return:
 *  IDBUS_PIO_OK -> activated tx mode properly
 *  IDBUS_PIO_INVALID_PARAMS -> invalid chip configuration
 *  IDBUS_PIO_NO_FREE_SM -> no free state machine found
 */
int __idbus_active_tx(uint8_t id);

/*
 * Prepare the idbus instance pio for reading
 *
 * params:
 *  id: uint8_t -> instance id
 *
 * return:
 *  IDBUS_PIO_OK -> activated rx mode properly
 *  IDBUS_PIO_INVALID_PARAMS -> invalid chip configuration
 *  IDBUS_PIO_NO_FREE_SM -> no free state machine found
 */
int __idbus_active_rx(uint8_t id);

/*
 * Raw idbus sending function
 *
 * params:
 *  id: uint8_t -> instance id
 *
 * return:
 *  IDBUS_PIO_OK -> send out idbus msg properly
 *  IDBUS_PIO_ERROR -> received invalid checksum code from pio code
 */
int __raw_write_idbus_msg(uint8_t id);

/*
 * Receive raw idbus messages
 * This function will read the idbus message from the physical layer.
 * It is important to understand that the timeout value, which is passed via
 * params, only affects the first message byte and not the intermediate bytes.
 * Hence, if the function returns a timeout error, no byte was received at all.
 *
 * params:
 *  id: uint8_t -> instance id
 *  timeout: uint32_t -> define the timeout in us
 *                       if -1 is passed, no timeout is used
 *                       for receiving the next message
 *
 * return:
 *  IDBUS_PIO_OK -> recv idbus msg properly
 *  IDBUS_PIO_TIMEOUT -> run into next message timeout
 */
int __raw_read_idbus_msg(uint8_t id, uint32_t timeout);

/*
 * Internal helper function to print the idbus instance buffer
 * if the instance id is not value, nothing will be print
 *
 * params:
 *  fd: int -> instance id to print internal buffer for
 */
void __idbus_print_buffer(uint8_t id);

/*
 * Helper function for printing a idbus_pio message
 * if the msg is not valid, nothing will be print
 *
 * params:
 *  msg: idbus_msg_t* -> message to print
 */
void idbus_print_msg(idbus_msg_t *msg);

#endif