set(myprojectname "spi_flash_transaction")

cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${myprojectname} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}.pio)
target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib hardware_pio pico_multicore)
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
