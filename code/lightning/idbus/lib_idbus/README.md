# idbus_core

idbus_core is a library which provides a set of functionalities to does with the Apple internal IDBUS protocol. It allows you to define either a HiFive or Tristar chip based on there metadata such as accessory serial number and therefore removes the need to work with plain IDBUS messages. Based on this metadata, either a HiFive of Tristar instance can be initiated, which contains the entire information to perform a handshake successfully.

Therefore, this library is designed to give the IDBUS messages names and make them more human understandable, instead of dealing with the raw physical signals. The communication is not done by this lib, however it is designed to easily interact with them via the abstraction struct:

```c
typedef struct
{
    int (*idbus_open_hifive)(int *fd, void *param);
    int (*idbus_open_tristar)(int *fd, void *param);
    int (*write_idbus)(int fd, idbus_msg_t *msg);
    int (*read_idbus)(int fd, idbus_msg_t *msg);
    int (*idbus_reset)(int fd);
    int (*close_idbus)(int fd);
} idbus_core_phy_t;
```

So far, the library can leverage the following libraries to interact with the physical IDBUS layer:
* [idbus_io](./idbus_io/../README.md)

However, any library can be leveraged, as long as the interface from the `idbus_core_phy_t` is fulfilled. Please have a look at [idbus_io_link.c](./idbus_io_link.c) to see how the [idbus_io](./idbus_io/../README.md) library got integrated into idbus_core.

## Define chips
idbus_core allows you to define the chips (Tristar or HiFive) based on there metadata/configuration, as shown below:
```c
// struct holding all the information about a HiFive chip
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
    seven_three_msg_t seven_three_msg;
} idbus_hifive_info_t;

// struct holding all the information about a Tristar chip
typedef struct
{
    seven_four_msg_t seven_four_msg;
    charging_state_msg_t charing_active;
    charging_state_msg_t charing_deactivated;
    handshake_summary_msg_t handshake_summary_msg;
} idbus_tristar_info_t;
```

## Perform handshakes
Performing handshake is quite easily, simply define the chip, initiate it and let the library handle the rest.

### HiFive
Please have a look at the following example: [hifive.c](./../apps/hifive.c)
```c
// some board and app initialization

// initialize the idbus_core library
err = idbus_init();

// prepare a new HiFive instance via idbus_core lib
// define the chip info in a separate function
err = load_hifive_info(&hifive_info);

// initiate idbus_core lib with the loaded HiFive instance
err = idbus_init_hifive(&hifive, &hifive_info);

// perform idbus handshake for loaded instance
err = idbus_do_handshake_hifive(&hifive, (void *)IDBUS_PIN, NULL);
```

### Tristar
Please have a look at the following example: [tristar.c](./../apps/tristar.c)
```c
// some board and app initialization

// initialize the idbus_core library
err = idbus_init();

// prepare a new tristar instance via idbus_core lib
// define the chip info in a separate function
err = load_tristar_info(&tristar_info);

// initiate idbus_core lib with the loaded tristar instance
err = idbus_init_tristar(&tristar, &tristar_info);

// perform idbus handshake for loaded instance
err = idbus_do_handshake_tristar(&tristar, (void *)IDBUS_PIN, NULL);
```