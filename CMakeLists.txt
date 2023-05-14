cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico_w)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pull in SDK (must be before project)
include(cmake/pico_sdk_import.cmake)
include(cmake/pico_extras_import_optional.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(vacuum-meter-bt LANGUAGES ASM C CXX VERSION 2.0.0)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

add_executable(${PROJECT_NAME}
        ${PROJECT_NAME}.cpp
)

add_subdirectory(libs)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(${PROJECT_NAME} 
        pico_stdlib
        hardware_i2c
        hardware_adc
)

if (WIFI)
        target_link_libraries(${PROJECT_NAME} 
                pico_cyw43_arch_none 
        )
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
