/*!
 * @file /fifo.h
 * @brief PIO state machine FIFOs
 * @details
 * Supports write & pull, push & read, and joining IN & OUT FIFOs to create one larger on, along with status 
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stdbool.h>

#define TOTAL_FIFO_SIZE_PER_SM 8

typedef enum { BIDI, RX_ONLY, TX_ONLY } fifo_mode_t;

typedef enum { FIFO_FULL, FIFO_EMPTY, FIFO_HAS_DATA } fifo_state_t;

typedef enum { TX_STATE, RX_STATE, TX_CONTENTS, RX_CONTENTS, FIFO_MATCH } fifo_compare_t;

typedef struct {
    /* public */
    uint32_t      buffer[TOTAL_FIFO_SIZE_PER_SM];
    fifo_mode_t   mode;
    fifo_state_t  rx_state;
    fifo_state_t  tx_state;
    uint32_t      status;
    bool          EXECCTRL_STATUS_SEL;
    int           N;
    /* private */
    int           rx_bottom;
    int           rx_top;
    int           tx_bottom;
    int           tx_top;
} fifo_t;

/* Note regarding status:
 * if !EXECCTRL_STATUS_SEL then status is All-ones if TX FIFO level < N, otherwise all-zeroes
 * else EXECCTRL_STATUS_SEL so then status is All-ones if RX FIFO level < N, otherwise all-zeroes
 * default is EXECCTRL_STATUS_SEL, N=4
 */

void fifo_init(fifo_t * fifo, fifo_mode_t mode);
bool fifo_write(fifo_t * fifo, uint32_t value);
bool fifo_read(fifo_t * fifo, uint32_t * value_ptr);
bool fifo_push(fifo_t * fifo, uint32_t value);
bool fifo_pull(fifo_t * fifo, uint32_t * value_ptr);

void fifo_copy(fifo_t * from, fifo_t * to);
fifo_compare_t fifo_compare(fifo_t * from, fifo_t * to);

/* Notes:
 * 1) In bidi mode, first half is the rx fifo and the second half is the tx fifo
 * 2) read and write are user (consumer) operations, push and pull are SM (client) operations
 * 3) all functions return true if successful and false if can't be done (like full, empty, or mode issue)
 *    (can status fields to get more insight into failure)
 * 4) changing the mode directly instead of going through the init function doesn't properly (re)configure the fifo
 * 5) status corresponds to PIO STATUS (either zero or all ones, as determined by EXECCTRL_STATUS_SEL)
 */

#endif