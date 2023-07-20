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
#include "spi_flash_pio.pio.h"

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

#define MAX_TRACE 2000
#define  TIME_TRACE 1000000

#define CLK_MASK (1 << CLK_PIN)
#define  TX_MASK (1 <<  TX_PIN)
#define  RX_MASK (1 <<  RX_PIN)
#define  CS_MASK (1 <<  CS_PIN)
#define GPIO_MASK CLK_MASK + TX_MASK + RX_MASK + CS_MASK

#define FLASH_PAGE_SIZE        256
#define FLASH_SECTOR_SIZE      4096

#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_READ         0x03
#define FLASH_CMD_STATUS       0x05
#define FLASH_CMD_WRITE_EN     0x06
#define FLASH_CMD_SECTOR_ERASE 0x20

#define FLASH_CMD_READ_MANUFACTURER_ID       0X90

#define FLASH_STATUS_BUSY_MASK 0x01


static uint32_t trace[MAX_TRACE];
static int trace_index;

void print_trace_pin (uint32_t pins) {
    if (pins & CLK_MASK) printf("CLK: H ");
    else printf("CLK: L ");
    if (pins & TX_MASK) printf("TX: H ");
    else printf("TX: L ");
    if (pins & RX_MASK) printf("RX: H ");
    else printf("RX: L ");
    if (pins & CS_MASK) printf("CS: H ");
    else printf("CS: L ");
}

void print_trace_pin_diff (uint32_t pins, uint32_t last) {

    if ((pins ^ last) & CLK_MASK) {
        if (pins & CLK_MASK) printf("CLK: H ");
        else printf("CLK: L ");
    }
    else printf("CLK:   ");

    if ((pins ^ last) & TX_MASK) {
        if (pins & TX_MASK) printf("TX: H ");
        else printf("TX: L ");
    }
    else printf("TX:   ");

    if ((pins ^ last) & RX_MASK) {
        if (pins & RX_MASK) printf("RX: H ");
        else printf("RX: L ");
    }
    else printf("RX:   ");

    if ((pins ^ last) & CS_MASK) {
        if (pins & CS_MASK) printf("CS: H ");
        else printf("CS: L ");
    }
    else printf("CS:   ");
    
}

void print_trace(uint32_t start, uint32_t end, uint32_t after) {
    printf("\n");
    printf("trace table:\n");
    printf("-------------------------%d\n", start);
    print_trace_pin(trace[0]);
    printf("%d\n", trace[1]);
    for (int i=2; i<trace_index; i+=2) {
        print_trace_pin_diff(trace[i], trace[i-2]);
        printf("%d\n", trace[i+1]);
    }
    printf("-------------------------%d\n", end);
    printf("xxxxxxxxxxxxxxxxxxxxxxxxx%d\n", after);
}

void up1() {
    uint32_t time_start, time_now, gpios_now, gpios_last, gpios_diff;
    time_start = time_us_32();
    trace_index = 0;
    printf("tracing ...\n");
    gpios_last = gpio_get_all() & GPIO_MASK;
    trace[trace_index++] = gpios_last;
    trace[trace_index++] = time_start;
    do {
        gpios_now = gpio_get_all() & GPIO_MASK;
        gpios_diff = gpios_now ^ gpios_last;
        if (gpios_diff && trace_index < MAX_TRACE) {
            trace[trace_index++] = gpios_now;
            trace[trace_index++] = time_now;
        }
        gpios_last = gpios_now;
        time_now = time_us_32();
    } while ( (time_now - time_start) < TIME_TRACE );
    if (trace_index == MAX_TRACE) printf("exceeded MAX_TRACE\n");
    printf("end tracing, captured: %d\n", trace_index/2);
    while(true) sleep_ms(5000);
}

/*
    send prefix + cmd + address
    prefix = cmd size (either 16 or 32) in highest 6 bits, followed by read flag, reserved bit, and 24 bits of number of bits to read or write
    then read or write however many full 4 byte sets from buffer there are
    lastly read or write however many leftover bytes after full 4 byte sets have been sent
 */
void __not_in_flash_func(flash_transaction)(uint8_t flash_cmd, uint32_t addr24, uint8_t bytes_in_cmd, bool read_bit, uint32_t num_bytes, uint8_t * buffer, uint cs_pin) {
    uint32_t prefix, cmd;
    uint8_t i, j;
    int fullwords, leftover, bytes_processed;
    prefix = bytes_in_cmd * 8;
    prefix = (prefix << 1) + read_bit;
    prefix = (prefix << 25) + (num_bytes * 8);
    cmd = flash_cmd;
    cmd = (cmd << 24) + (addr24  & 0XFFFFFF);
    pio_sm_put_blocking(TRANSACTION_PIO, TRANSACTION_SM, prefix);    
    pio_sm_put_blocking(TRANSACTION_PIO, TRANSACTION_SM, cmd);
    fullwords = num_bytes / 4;
    leftover = num_bytes % 4;
    bytes_processed = 0;
    if (read_bit) {
        for (i=0; i<fullwords; i++) {
            prefix = pio_sm_get_blocking(TRANSACTION_PIO, TRANSACTION_SM);
            for (j=0; j<4; j++) buffer[bytes_processed++] = prefix >> (24 - j*8);
        }
        if (leftover) {
            prefix = pio_sm_get_blocking(TRANSACTION_PIO, TRANSACTION_SM);
            for (j=0; j<leftover; j++) buffer[bytes_processed++] = prefix >> ((24-leftover*8) - j*8);
        }
        else { /* get final push and just discard it since it won't have anything in it */
            prefix = pio_sm_get_blocking(TRANSACTION_PIO, TRANSACTION_SM);
        }
     }
     else {
        for (i=0; i<fullwords; i++) {
            prefix = 0;
            for (j=0; j<4; j++) prefix = prefix + ( buffer[bytes_processed++] << (24 - j*8) );
            pio_sm_put_blocking(TRANSACTION_PIO, TRANSACTION_SM, prefix);
        }
        if (leftover) {
            prefix = 0;
            for (j=0; j<leftover; j++) prefix = prefix + ( buffer[bytes_processed++] << (24 - j*8) );
            pio_sm_put_blocking(TRANSACTION_PIO, TRANSACTION_SM, prefix);
        }
    }
}

void __not_in_flash_func(flash_read)(uint cs_pin, uint32_t addr, uint8_t *buf, size_t len) {
    flash_transaction(FLASH_CMD_READ, addr, 4, 1, len, buf, cs_pin);
}

void __not_in_flash_func(flash_write_enable)(uint cs_pin) {
    flash_transaction(FLASH_CMD_WRITE_EN, 0, 1, 0, 0, NULL, cs_pin);
}

void __not_in_flash_func(flash_wait_done)(uint cs_pin) {
    uint8_t status;
    uint8_t status_response[2];
    do {
        flash_transaction(FLASH_CMD_STATUS, 0, 1, 1, 2, status_response, cs_pin);
        status = status_response[1];
    } while (status & FLASH_STATUS_BUSY_MASK);
}

void __not_in_flash_func(flash_sector_erase)(uint cs_pin, uint32_t addr) {
    flash_write_enable(cs_pin);
    flash_transaction(FLASH_CMD_SECTOR_ERASE, addr, 4, 0, 0, NULL, cs_pin);
    flash_wait_done(cs_pin);
}

void __not_in_flash_func(flash_page_program)(uint cs_pin, uint32_t addr, uint8_t data[]) {
    flash_write_enable(cs_pin);
    flash_transaction(FLASH_CMD_PAGE_PROGRAM, addr, 4, 0, FLASH_PAGE_SIZE, data, cs_pin);
    flash_wait_done(cs_pin);
}

uint16_t __not_in_flash_func(read_manufacturer_id)(uint cs_pin) {
    uint8_t id_bytes[2]; 
    uint16_t id;
    flash_transaction(FLASH_CMD_READ_MANUFACTURER_ID, 0, 4, 1, 2, id_bytes, cs_pin);
    id = (id_bytes[0] << 8) + id_bytes[1];
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

    //multicore_launch_core1(up1); /* start trace */
    sleep_ms(1);
    printf("SPI initialised, let's goooooo\n");

    spi_flash_transaction_init(TRANSACTION_PIO, TRANSACTION_SM, CLK_PIN, TX_PIN, RX_PIN, CS_PIN);


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
