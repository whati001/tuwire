# define the library
add_library(lib_iap
    iap_core.c
    iap_io.c
)

# add all local header files to the idbus_core_lib build
target_include_directories(lib_iap PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

if(${IAP_TARGET} STREQUAL "RPI")
    # pull in common dependencies libs
    target_link_libraries(lib_iap PRIVATE
        pico_stdlib
    )

    # enable usb output, disable uart output
    pico_enable_stdio_usb(lib_iap 1)
    pico_enable_stdio_uart(lib_iap 0)
elseif(${IAP_TARGET} STREQUAL "ZEPHYR")
    message(STATUS "Build IAP_CORE for a Zephyr board")
    message(FATAL_ERROR "Zephyr build is not implemented yet")
else()
    message(FATAL_ERROR "Unkown IAP_TARGET sepcified, currently the following are supported: [RPI,ZEPHYR]")
endif()

# source the proper IO
add_subdirectory(io)

# source the proper cp
add_subdirectory(cp)

# source the proper core
add_subdirectory(core)

# source hid
add_subdirectory(hid)
