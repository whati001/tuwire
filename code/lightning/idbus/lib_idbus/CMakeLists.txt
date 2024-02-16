# define the library
add_library(lib_idbus
    idbus.c
    idbus_hifive.c
    idbus_tristar.c
    crc.c
)

# pull in common dependencies libs
target_link_libraries(lib_idbus PRIVATE
    pico_stdlib
)

# add all local header files to the idbus_lib build
target_include_directories(lib_idbus PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# include the desired io library, set by $IDBUS_CORE_PHY
set(IDBUS_CORE_PHY "IDBUS_PIO")
add_subdirectory(io)

# enable usb output, disable uart output
pico_enable_stdio_usb(lib_idbus 1)
pico_enable_stdio_uart(lib_idbus 0)
