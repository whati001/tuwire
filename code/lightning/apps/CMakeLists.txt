add_executable(usb_otg
    usb_otg.c

    # iap_cp_gw.c
    iap_hid_gw.c
)

# link the executable using the IR transmit and receive libraries
target_link_libraries(usb_otg
    pico_stdlib
    lib_idbus
    lib_iap
    hardware_i2c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(usb_otg 1)
pico_enable_stdio_uart(usb_otg 0)

pico_add_extra_outputs(usb_otg)

# -----------------------------------------------------------------------------
add_executable(lightning_condom
    lightning_condom.c
)

target_link_libraries(lightning_condom
    lib_idbus
)

# enable usb output, disable uart output
pico_enable_stdio_usb(lightning_condom 1)
pico_enable_stdio_uart(lightning_condom 0)

pico_add_extra_outputs(lightning_condom)
