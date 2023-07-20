set(myprojectname "spi_flash")
set(libraries "pico_stdlib" "hardware_spi")
# default SPI0 appears to be the lower right block: sck=18, tx=19, rx=16, cs=17
# add_compile_definitions(PICO_DEFAULT_SPI_SCK_PIN=2 PICO_DEFAULT_SPI_TX_PIN=3 PICO_DEFAULT_SPI_RX_PIN=4 PICO_DEFAULT_SPI_CSN_PIN=5)

cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${myprojectname} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)
target_link_libraries(${PROJECT_NAME} ${libraries})
pico_add_extra_outputs(${PROJECT_NAME})
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
