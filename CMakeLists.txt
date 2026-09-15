cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(traffic_lights C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(traffic_lights src/main.cpp src/usb.cpp src/usb_with_watchdog.cpp src/usb_descriptors.c src/gpio_classes.cpp src/traffic_classes.cpp
    src/transition.cpp src/comms.cpp )
target_link_libraries(traffic_lights pico_stdlib)
pico_add_extra_outputs(traffic_lights)

target_include_directories(traffic_lights PRIVATE
    src/
)

target_link_libraries(traffic_lights
    pico_stdlib
    tinyusb_device   
    tinyusb_board    
    hardware_adc
    hardware_dma
    hardware_watchdog
    hardware_gpio
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)