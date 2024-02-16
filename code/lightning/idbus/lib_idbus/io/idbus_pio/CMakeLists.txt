# use idbus_io for communication
target_sources(lib_idbus PUBLIC
    idbus_pio_link.c
    idbus_pio.c
)

# add some PIO code
pico_generate_pio_header(lib_idbus ${CMAKE_CURRENT_LIST_DIR}/idbus_pio_rx.pio)
pico_generate_pio_header(lib_idbus ${CMAKE_CURRENT_LIST_DIR}/idbus_pio_tx.pio)
pico_generate_pio_header(lib_idbus ${CMAKE_CURRENT_LIST_DIR}/idbus_pio_wake.pio)

# pull in common dependencies libs
target_link_libraries(lib_idbus PUBLIC
    hardware_pio
    pico_stdlib
)

# add all local header files to the idbus_io build
target_include_directories(lib_idbus PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# enable usb output, disable uart output
pico_enable_stdio_usb(lib_idbus 1)
pico_enable_stdio_uart(lib_idbus 0)
