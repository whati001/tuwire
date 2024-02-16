
add_definitions(-DIAP_VERSION=${IAP_VERSION})

# source based on some definitions
# we use UART for the io communication
target_sources(lib_iap PRIVATE
    iap_impl.c
)

# add all local header files to the idbus_core_lib build
target_include_directories(lib_iap PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)