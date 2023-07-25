cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(basement_project C CXX ASM)

pico_sdk_init()

add_executable(basement_project
        main.cpp
    )

pico_enable_stdio_usb(basement_project 1)
pico_enable_stdio_uart(basement_project 0)

target_link_libraries(basement_project pico_stdlib hardware_uart hardware_rtc pico_multicore hardware_irq pico_bootsel_via_double_reset hardware_watchdog) #hardware_spi hardware_i2c
pico_add_extra_outputs(basement_project)