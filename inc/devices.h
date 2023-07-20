#ifndef DEVICE_H
#define DEVICE_H

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

void devices_enable_spi_flash(uint8_t clk_pin, uint8_t tx_pin, uint8_t rx_pin, uint8_t cs_pin);

#endif