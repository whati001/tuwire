# define which IO implementation should be used
if(${IDBUS_CORE_PHY} STREQUAL "IDBUS_PIO")
    add_subdirectory(idbus_pio)
else()
    message(FATAL_ERROR, "Currently only IDBUS_PIO is supported for IO, please verify variable IDBUS_CORE_PHY")
endif()