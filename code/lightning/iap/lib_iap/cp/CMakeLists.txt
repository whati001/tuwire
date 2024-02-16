
target_include_directories(lib_iap PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# source files based on the variable IAP_CP
target_sources(lib_iap PRIVATE
    iap_cp.c
)

# source the correct header file
# this header file contains all the register addresses
if(${IAP_CP} STREQUAL "IAP_2B")
    # we use the CP 2.0B or 2.0C with i2c communicaiton
    add_subdirectory(v2)
else()
    message(FATAL_ERROR, "Currently only IAP_2B is supported")
endif()

add_subdirectory(io)