add_executable(idbus_hifive
    hifive.c
)

# link the executable using the IR transmit and receive libraries
target_link_libraries(idbus_hifive
    pico_stdlib
    lib_idbus
)

# set the define to include correct IDBUS messages
target_compile_definitions(idbus_hifive PRIVATE
    -DAPPLE_USB)

# enable usb output, disable uart output
pico_enable_stdio_usb(idbus_hifive 1)
pico_enable_stdio_uart(idbus_hifive 0)

pico_add_extra_outputs(idbus_hifive)

# -----------------------------------------------------------------------------
# # TODO: update code to use official idbus_core APIs instead of idbus_pio.h
# add_executable(idbus_hifive_fuz
# hifive_fuzzing.c
# )

# # link the executable using the IR transmit and receive libraries
# target_link_libraries(idbus_hifive_fuz
# pico_stdlib
# lib_idbus
# )

# # set the define to include correct IDBUS messages
# target_compile_definitions(idbus_hifive_fuz PRIVATE
# -DAPPLE_USB)

# # enable usb output, disable uart output
# pico_enable_stdio_usb(idbus_hifive_fuz 1)
# pico_enable_stdio_uart(idbus_hifive_fuz 0)

# pico_add_extra_outputs(idbus_hifive_fuz)

# -----------------------------------------------------------------------------
add_executable(idbus_tristar
    tristar.c
)

# link the executable using the IR transmit and receive libraries
#
target_link_libraries(idbus_tristar
    pico_stdlib
    lib_idbus
)

# set the define to include correct IDBUS messages
target_compile_definitions(idbus_tristar PRIVATE
    -DAPPLE_USB)

# enable usb output, disable uart output
pico_enable_stdio_usb(idbus_tristar 1)
pico_enable_stdio_uart(idbus_tristar 0)

pico_add_extra_outputs(idbus_tristar)

# -----------------------------------------------------------------------------
# TODO: update code to use official idbus_core APIs instead of idbus_pio.h
# add_executable(idbus_copycat
# copycat.c
# )

# # link the executable using the IR transmit and receive libraries
# target_link_libraries(idbus_copycat
# pico_stdlib
# lib_idbus
# )

# # set the define to include correct IDBUS messages
# target_compile_definitions(idbus_copycat PRIVATE
# -DAPPLE_USB)

# # enable usb output, disable uart output
# pico_enable_stdio_usb(idbus_copycat 1)
# pico_enable_stdio_uart(idbus_copycat 0)

# pico_add_extra_outputs(idbus_copycat)

# -----------------------------------------------------------------------------
# TODO: update code to use official idbus_core APIs instead of idbus_pio.h
# add_executable(idbus_proxy
# proxy.c
# )

# # link the executable using the IR transmit and receive libraries
# target_link_libraries(idbus_proxy
# pico_stdlib
# lib_idbus
# )

# # enable usb output, disable uart output
# pico_enable_stdio_usb(idbus_proxy 1)
# pico_enable_stdio_uart(idbus_proxy 0)

# pico_add_extra_outputs(idbus_proxy)