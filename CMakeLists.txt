cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(time_switch C CXX ASM)

pico_sdk_init()
add_subdirectory(modules/one_wire)

add_executable(time_switch
        main.cpp
    )

pico_enable_stdio_usb(time_switch 1)
pico_enable_stdio_uart(time_switch 0)

target_link_libraries(time_switch 
pico_stdlib hardware_uart hardware_rtc 
pico_multicore hardware_watchdog pico_time
pico_one_wire hardware_i2c

) #hardware_spi hardware_i2c
pico_add_extra_outputs(time_switch)