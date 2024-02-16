# source files based on the variable IAP_IO

# add all local header files to the idbus_core_lib build
target_include_directories(lib_iap PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

if(${IAP_TARGET} STREQUAL "RPI")
    # source based on some definitions
    target_sources(lib_iap PRIVATE
        rpi_uart.c
        rpi_usb.c
    )

    target_link_libraries(lib_iap PRIVATE
        hardware_uart
    )

elseif(${IAP_TARGET} STREQUAL "ZEPHYR")
    message(STATUS "For Target ZEPHYR we will use USB for the iAP communication")
    target_sources(lib_iap PRIVATE
        zephyr_uart.c
        zephyr_usb.c
    )
else()
    message(FATAL_ERROR "Currently only IAP_UART is supported")
endif()
