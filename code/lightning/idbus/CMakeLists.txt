cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# define the project to build
project(idbus_emulation C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# include local libraries
add_subdirectory(lib_idbus)

# include the app executables
add_subdirectory(apps)