# source files based on the variable IAP_IO

# add all local header files to the idbus_core_lib build
target_include_directories(lib_iap PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

if(${IAP_TARGET} STREQUAL "RPI")
    # source based on some definitions
    target_sources(lib_iap PUBLIC
        rpi_io.c
    )

    target_link_libraries(lib_iap PRIVATE
        hardware_i2c
        hardware_gpio
    )

elseif(${IAP_TARGET} STREQUAL "ZEPHYR")
    target_sources(lib_iap PRIVATE
        zephyr_io.c`
    )
else()
    message(FATAL_ERROR "Currently only IAP_UART is supported")
endif()
