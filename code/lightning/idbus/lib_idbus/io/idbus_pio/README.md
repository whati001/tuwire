# idbus_io

idbus_io is a library which provides a set of functionalities to communicate with Apple devices through the Lightning cables by emulating the [IDBUS](https://nyansatan.github.io/lightning/) protocol. The library leverages for the IDBUS communication the embedded Programmable I/O (PIO) engine, therefore, concurrent access is not possible yet. The library allows to allocated multiple instance of the `idbus_io_t`, but the read and write of all these instances needs to be synchronized. The library does not guarantee this synchronization yet, hence, the developer needs to ensure it.

The PIO code is highly highly inspired by the [TamarinCable](https://github.com/stacksmashing/tamarin-firmware), but was significantly optimized to meet all timing constrains of the IDBUS protocol.

## Installation
To use the library, please install the Pico C/C++ SDK and add the library to your project, by adding the following lines to the `CMakeList.txt` file:
```cmake
# include the idbus_io library
add_subdirectory(idbus_io)

# link the library to your target
add_executable(idbus_example
    example.c
)

# link the executable using the IR transmit and receive libraries
#
target_link_libraries(idbus_example
    idbus_io_lib # link idbus_io lib
    # ... all other libs
)
```

## Usage
The library design reassembles the idea of the file system, and follows the `open/read/write/close` pattern.
Please find below a short example, how the library allows to emulate messages either as a HIFIVE and TRISTAR chip.

### Interface
The interface between the user code and the library consists of an file descriptor and a message buffer.
For the file descriptor, an integer value is leveraged, and for the message buffer, the following struct is available:
```c
typedef struct
{
    uint8_t header;
    uint8_t *data_ptr;
    uint8_t data_len;
    uint8_t crc;
} idbus_msg_t;
```

As you can see, the struct requires an external buffer, in which the message data will be stored and is linked to the struct via the `data_ptr` member. To maintain a variable size of data per message, the struct does not come with a fixed data buffer. Additionally, the current design saves memory as well. However, this requires to allocate a separate buffer and link it to the struct, as shown in the example below:

```c
// allocate 
uint8_t msg_buf[IDBUS_IO_DATA_BUFFER_SIZE];
idbus_msg_t msg = {
    .header = 0, 
    .data_ptr = (uint8_t *)&msg_buf, 
    .data_len = IDBUS_IO_DATA_BUFFER_SIZE, 
    .crc = 0
    };
```

The example above allocates an idbus message in user space, with a data buffer size of `IDBUS_IO_DATA_BUFFER_SIZE`. This is a quite good idea, because the library maintains an internal buffer per instance with a size of `IDBUS_IO_DATA_BUFFER_SIZE + 2`, for sending and reading. The additional two bytes provide enough space to store the header and CRC byte next to the actual data. The internal buffer is mandatory to achieve a constant sending time, because otherwise, the header/CRC data could be stored on a different memory page than the data, which will affect the loading time significantly.

### HIFIVE
To emulate a HIFIVE chip, please open a new idbus_io instance and pass as chip type `HIFIVE`.
It's important, that the first response (`write_idbus_io`) is send within ~2.2ms, otherwise TRISTAR will resend the first request.
However, it seems like, if we have answered to the first request, we have more than 100ms to response to all other requests. Therefore, ensure that the first response is very fast.
```c
// allocate a message buffer in user space
// idbus_msg_t msg = ...

// open a new idbus_io instance and act as a HIFIVE chip
// use for the communication the GPIO pin <gpio_pin>
err = open_idbus_io(&fd, HIFIVE, gpio_pin);
CHECK_ERROR(err);

// read IDBUS message from TRISTAR
err = read_idbus_io(fd, &msg);
CHECK_ERRROR(err);
idbus_print_msg(&msg);

// handle the received IDBUS, but be fast to meet the timing requirements
// and update the msg to hold the response
err = write_idbus_io(fd, &msg);
CHECK_ERRROR(err);
idbus_print_msg(&msg);

// close the idbus_io instance
err = close_idbus_io(fd);
CHECK_ERROR(err);
```


### TRISTAR
To emulate a TRISTAR chip, please open a new idbus_io instance and pass as chip type `TRISTAR`.
```c
// allocate a message buffer in user space
// idbus_msg_t msg = ...

// open a new idbus_io instance and act as a TRISTAR chip
// use for the communication the GPIO pin <gpio_pin>
fd = open_idbus_io(TRISTAR, gpio_pin);
CHECK_ERROR(fd);

// write the IDBUS message
err = write_idbus_io(fd, &msg);
CHECK_ERRROR(err);
idbus_print_msg(&msg);

// read IDBUS message from TRISTAR
err = read_idbus_io(fd, &msg);
CHECK_ERRROR(err);
idbus_print_msg(&msg);

// close the idbus_io instance
err = close_idbus_io(fd);
CHECK_ERROR(err);
```

## Implementation
This section tries to explain, why the library was design as it is.

### Internal buffer
The library leverages an internal message buffer, which is used for the internal read and write procedure. The idea behind this is to make the library more stable and avoid null pointer exceptions in case the user unallocated the buffer in user land. Furthermore, the buffer "ensures" that the entire message data is stored close to each other and not cache misses during the read occur. The initial implementation has used an `idbus_msg_t` inside the `idbus_io_t` struct as a buffer. Unfortunately, we have seen some timing differences during the write processes, after sending the header and reading the buffer info. We assume this was because the buffer was not in the cache. So the logic was changed to store everything within the same buffer array.

### PIO-Write
The PIO code could be simplified, if we could directly shift out the bits via the `OUT` instruction. However, this is not possible, because each bit starts with at the `HIGH` level. Also autopull does not work, because the `!OSRE` is always `TRUE` if the FIFO includes data. Hence, we are not able identify if a STOP preamble needs to be send or not. This is possible without the autopull, because after 8-bit, the `!OSRE` evaluates to `FALSE`.

### PIO-Read
For reading bytes from the IDBUS line, we have disabled autopush, because otherwise we are not able to encode an BREAK value (needs to differ from 0x00-0xff). We have tried to leverage the IRQ for encoding the BREAK signal, unfortunately without success. The issue we see here is synchronization. If we set the IRQ within the PIO code, the driver needs to process and acknowledge the IRQ before the next byte is send. Because if we notice that a IRQ is set, do do not know, if the byte within the FIFOs still belong to the IDBUS message or not. This could be fixed, by simply waiting within the PIO code, until the IRQ got acknowledger by the driver code, however, if the driver is to slow, we miss bits/bytes, hence, also not an ideal solution. Therefore, we stick with the solution to encode the BREAK into the FIFO stream, which implies that we are not able to use autopush.
Also to use timeouts was explored. But if we use timeouts globally (HiFive & Tristar) we are to slow for sending the HiFive response (only ~2ms for the 0x75 res). 

### PIO-Combined
We have tried to combine the TX and RX code into a single PIO logic and jump the the correct code section, unfortunately the result was quite unstable. We assume this is because we had to strip away a lot of safety instructions like clearing the FIFOS, OSRS etc. We assume, that it would be possible, however this would require further research and it is not mandatory to implement the IDBUS protocol properly. In addition, the code would be more complex and error prone. 

### PIO-HIFIVE
For the HIFIVE chip emulation, a PIO for TX and RX exists.
The TX code pulls first the amount of bytes, which are needed to send out from the TX FIFO and stores it in the scratch register X. Next, it pulls as long as the OSR is not empty from it and writes it to the phy layer. The OSR will be empty after exactly 8 bits, because this is the size we passed to the `sm_config_set_out_shift` function as size. An empty OSR signals, that the code needs to send a STOP signal and decrements the amount of bytes to send out. Now it pulls the next byte and continues sending out the bits. This loop of sending out the bytes continues until the register X is zero, which indicates the end of the sending procedure. Now we pass a one via the RX FIFO to the driver code to signal that the sending is done and the driver can continue with it's work. Without this success signal, a race condition could occur, in case the driver code tries to send a new message even though the old one is still sending.

Conversely to the sending code, the receiving code does not know how many bytes it will receive. However, TRISTAR is sending so called BREAK signals, which indicate the start and the end of a message. But we need to encode the BREAK in such a way that the driver code understands it properly. Therefore, the first task of the RX code is to pull the BREAK value mask from the TX FIFO and store it in the Y register. This BREAK value should be clearly distinguishable, hence, use a value which is higher than 0xff. Now it starts receiving bytes and maintains a bit counter in the X register. After receiving eight bits, the code will push the value via the RX FIFO to the driver code. After this push, it will reset the X counter to eight and starts receiving the next byte. If a break is received, the code will send the BREAK marker and starts to receive a new bytes, which implies to reset the X register to eight. The driver code is now responsible to parse the message out of the BREAK + data + BREAK stream. The bit counter in X sounds quite odd, but it's mandatory, because otherwise we would not be able to encode the BREAK signal. For example, if we would enable autopush by setting the FIFO size to eight, the value range we are able to send is between 0x00 and 0xff, which does not allow us to encode uniquely the BREAK message, because the IDBUS range is also from 0x00 to 0xff.

### PIO-TRISTAR
For the TRISTAR chip emulation, we have not separated the TX from the RX, because of timing constrains. The official Lightning-to-USB cable send the response within approximately 20us. China cables even faster, which implies that a switch between TX to RX code would be to slow. Therefore, the TX code immediately starts reading the response and pushes it to the library code. The library stores the response within its buffer and only informs the user about a successfully write. When the user is calling the read function, the library simply returns the prior received message, without actually sampling the idbus line. This pretends the illusion that the write and read are independently, however, this is not the case. 

Because HiFive does not send a BREAK signal, Tristar uses a timeout logic to stop reading and waiting for new bytes on the IDBUS line.

# Credits
* [satan](https://nyansatan.github.io/lightning/)
* [stacksmashing](https://github.com/stacksmashing)