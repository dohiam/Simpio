/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Example of reading/writing an external serial flash using the PL022 SPI interface

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "spi_flash_basic.pio.h"

#define SENDER_PIO pio0
#define SENDER_SM  0

#define RECEIVER_PIO pio0
#define RECEIVER_SM  1

#define TRANSACTION_PIO pio0
#define TRANSACTION_SM  3
#define CLK_PIN 18
#define  TX_PIN  19
#define  RX_PIN  16
#define  CS_PIN  17

#define FLASH_PAGE_SIZE        256
#define FLASH_SECTOR_SIZE      4096

#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_READ         0x03
#define FLASH_CMD_STATUS       0x05
#define FLASH_CMD_WRITE_EN     0x06
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_READ_MANUFACTURER_ID       0X90

#define FLASH_STATUS_BUSY_MASK 0x01

void spi_write_blocking(uint8_t * bytes, int num_bytes) {
    int i;
    uint32_t value;
    PIO pio = SENDER_PIO;
    uint sm = SENDER_SM;
    for (i=0; i<num_bytes; i++) {
        value = bytes[i] << 24;
        pio_sm_put_blocking(pio, sm, value);
    }
    sleep_ms(1);
}

void spi_read_blocking(uint8_t * bytes, int num_bytes) {
    int i;
    PIO pio = RECEIVER_PIO;
    uint sm = RECEIVER_SM;
    uint32_t value;
    uint tx_level = pio_sm_get_tx_fifo_level(pio,sm);

    for (i=0; i<num_bytes; i++) {
        pio_sm_put_blocking(pio, sm, 0);
        value = pio_sm_get_blocking(pio, sm);
        bytes[i] = value;
    }
    tx_level = pio_sm_get_tx_fifo_level(pio,sm);
}

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

void __not_in_flash_func(flash_read)(uint cs_pin, uint32_t addr, uint8_t *buf, size_t len) {
    cs_select(cs_pin); 
    uint8_t cmdbuf[4] = {
            FLASH_CMD_READ,
            addr >> 16,
            addr >> 8,
            addr
    };
    spi_write_blocking(cmdbuf, 4);
    spi_read_blocking(buf, len);
    cs_deselect(cs_pin); 
}

void __not_in_flash_func(flash_write_enable)(uint cs_pin) {
    cs_select(cs_pin);
    uint8_t cmd = FLASH_CMD_WRITE_EN;
    spi_write_blocking(&cmd, 1);
    cs_deselect(cs_pin);
}

void __not_in_flash_func(flash_wait_done)(uint cs_pin) {
    uint8_t status;
    do {
        cs_select(cs_pin);
        uint8_t buf[2] = {FLASH_CMD_STATUS, 0};
        spi_write_blocking(buf, 2);
        spi_read_blocking(buf, 2);
        cs_deselect(cs_pin);
        status = buf[1];
    } while (status & FLASH_STATUS_BUSY_MASK);
}

void __not_in_flash_func(flash_sector_erase)(uint cs_pin, uint32_t addr) {
    uint8_t cmdbuf[4] = {
            FLASH_CMD_SECTOR_ERASE,
            addr >> 16,
            addr >> 8,
            addr
    };
    flash_write_enable(cs_pin);
    cs_select(cs_pin);
    spi_write_blocking(cmdbuf, 4);
    cs_deselect(cs_pin);
    flash_wait_done(cs_pin);
}

void __not_in_flash_func(flash_page_program)(uint cs_pin, uint32_t addr, uint8_t data[]) {
    uint8_t cmdbuf[4] = {
            FLASH_CMD_PAGE_PROGRAM,
            addr >> 16,
            addr >> 8,
            addr
    };
    flash_write_enable(cs_pin);
    cs_select(cs_pin);
    spi_write_blocking(cmdbuf, 4);
    spi_write_blocking(data, FLASH_PAGE_SIZE);
    cs_deselect(cs_pin);
    flash_wait_done(cs_pin);
}

uint16_t __not_in_flash_func(read_manufacturer_id)(uint cs_pin) {
    uint8_t id_bytes[2]; 
    uint16_t id;
    uint8_t cmdbuf[4] = {
            FLASH_CMD_READ_MANUFACTURER_ID,
            0,
            0,
            0
    };
    flash_wait_done(cs_pin);
    cs_select(cs_pin);
    spi_write_blocking(cmdbuf, 4);
    spi_read_blocking(id_bytes, 2);
    id = (id_bytes[0] << 8) + id_bytes[1];
    cs_deselect(cs_pin);
    return id;
}

void printbuf(uint8_t buf[FLASH_PAGE_SIZE]) {
    for (int i = 0; i < FLASH_PAGE_SIZE; ++i) {
        if (i % 16 == 15)
            printf("%02x\n", buf[i]);
        else
            printf("%02x ", buf[i]);
    }
}

static uint8_t page_buf[FLASH_PAGE_SIZE];

int main() {
    uint32_t start_time, end_time, after_time;
    const uint32_t target_addr = 0;

    stdio_init_all(); sleep_ms(5000);


    printf("SPI basic flash example\n");
    sender_init(SENDER_PIO, SENDER_SM, CLK_PIN, TX_PIN);
    receiver_init(RECEIVER_PIO, RECEIVER_SM, CLK_PIN, RX_PIN);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(CS_PIN);
    gpio_put(CS_PIN, 1);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(CS_PIN, "SPI CS"));

    printf("SPI initialised, let's goooooo\n");

    uint16_t id = read_manufacturer_id(CS_PIN);
    printf("manufacturer id = %04X\n", id);
    
    flash_sector_erase(CS_PIN, target_addr);
    flash_read(CS_PIN, target_addr, page_buf, FLASH_PAGE_SIZE);
    printf("After erase:\n");
    printbuf(page_buf);
    
    for (int i = 0; i < FLASH_PAGE_SIZE; ++i)
        page_buf[i] = i;
    flash_page_program(CS_PIN, target_addr, page_buf);
    flash_read(CS_PIN, target_addr, page_buf, FLASH_PAGE_SIZE);
    printf("After program:\n");
    printbuf(page_buf);

    flash_sector_erase(CS_PIN, target_addr);
    flash_read(CS_PIN, target_addr, page_buf, FLASH_PAGE_SIZE);
    printf("Erase again:\n");
    printbuf(page_buf);

    while(1) {printf("done\n"); sleep_ms(5000);}

    return 0;
}
