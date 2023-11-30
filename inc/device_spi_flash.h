/*!
 * @file /device_spi_flash.h
 * @brief SPI flash simulated device
 * @details
 * This configures and enables the simulated SPI flash device. This will add itself to the list of devices enabled in execution and ui (see execution.c and ui.c).
 * The enable function below is intended to be called by the parser when it encounters a command in the PIO program to configure this device
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef DEVICE_SPI_FLASH_H
#define DEVICE_SPI_FLASH_H

#include <stdint.h>

#define SPI_FLASH_PAGE_SIZE 256
#define SPI_FLASH_NUM_PAGES   3   

#define SPI_FLASH_DISPLAY_LINES 3 
#define SPI_FLASH_DISPLAY_LINE_SIZE 10 
// will display the first lines * line_size bytes/line of simulated flash storage contents 

#define SPI_FLASH_SIZE SPI_FLASH_PAGE_SIZE * SPI_FLASH_NUM_PAGES

#define SPI_SECTOR_SIZE 4096

// the following define how many times the spi_flash execution handler is called before it reports not busy
#define FLASH_CMD_PROGRAM_DELAY                10
#define FLASH_CMD_ERASE_DELAY                  10

void device_enable_spi_flash(uint8_t clk_pin, uint8_t tx_pin, uint8_t rx_pin, uint8_t cs_pin);

#endif
