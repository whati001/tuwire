target_include_directories(lib_iap PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# source files based on the variable IAP_CP
target_sources(lib_iap PRIVATE
    iap_hid.c
)