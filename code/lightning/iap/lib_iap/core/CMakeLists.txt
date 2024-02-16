# source files based on the variable IAP_VERSION

if(${IAP_VERSION} STREQUAL "IAP1")
    add_subdirectory(iap_one)
elseif(${IAP_VERSION} STREQUAL "IAP2")
    message(FATAL_ERROR, "iAP2 is currently not supported")
    # add_subdirectory(iap_two)
else()
    message(FATAL_ERROR, "Invalid value for IAP_VERSION found")
endif()
