cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# define the project to build
project(lightning_emulator C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-fno-omit-frame-pointer")


pico_sdk_init()

set(IAP_TARGET "RPI")
set(IAP_VERSION "IAP1")
set(IAP_CP "IAP_2B")



# include local libraries
add_subdirectory(idbus/lib_idbus)
add_subdirectory(iap/lib_iap)

# include the app executables
add_subdirectory(apps)