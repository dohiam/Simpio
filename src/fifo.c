/*!
 * @file /fifo.c
 * @brief PIO state machine FIFOs
 * @details
 * Supports write & pull, push & read, and joining IN & OUT FIFOs to create one larger on, along with status 
 *
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#include "fifo.h"

#define ALL_ONES  0xFFFFFFFF
#define ALL_ZEROS 0

#include "print.h"


void fifo_init(fifo_t * f, fifo_mode_t mode) {
    f->rx_state = FIFO_EMPTY;
    f->tx_state = FIFO_EMPTY;
    f->status = ALL_ONES;
    f->EXECCTRL_STATUS_SEL = true;
   switch(mode) {
       case BIDI:
           f->mode = BIDI;
           f->N = 4;
           f->rx_bottom = 0;
           f->rx_top = 0;
           f->tx_bottom = TOTAL_FIFO_SIZE_PER_SM / 2;
           f->tx_top = TOTAL_FIFO_SIZE_PER_SM / 2;
           break;
       case RX_ONLY:
           f->mode = RX_ONLY;
           f->N = 8;
           f->rx_bottom = 0;
           f->rx_top = 0;
           f->tx_bottom = TOTAL_FIFO_SIZE_PER_SM;
           f->tx_top = TOTAL_FIFO_SIZE_PER_SM;
           break;
       case TX_ONLY:
           f->mode = TX_ONLY;
           f->N = 8;
           f->rx_bottom = -1;
           f->rx_top = -1;
           f->tx_bottom = 0;
           f->tx_top = 0;
           break;
   };
}

static void set_status(fifo_t * f) {
    int depth;
    if (f->EXECCTRL_STATUS_SEL) { // status based on rx
       if (f->rx_state == FIFO_EMPTY) depth = 0;
       else depth = f->rx_top - f->rx_bottom;
       if (depth < f->N) f->status = ALL_ONES;
       else f->status = ALL_ZEROS;
    }
    else { // status based on tx
       if (f->tx_state == FIFO_EMPTY) depth = 0;
       else depth = f->tx_top - f->tx_bottom;
       if (depth < f->N) f->status = ALL_ONES;
       else f->status = ALL_ZEROS;
    }
}

/************************************************************************************************************************
 *
 * write: if room, put value on bottom of tx and shift everything up by one 
 * 
 ************************************************************************************************************************/

bool fifo_write(fifo_t * f, uint32_t value) {
   int i;
   if (f->mode == RX_ONLY) return false;
   else { 
       if (f->tx_state == FIFO_FULL) return false;
       for (i = f->tx_top-1; i >= f->tx_bottom; i--) f->buffer[i+1] = f->buffer[i];
       f->tx_top = f->tx_top + 1;
       if (f->tx_top == TOTAL_FIFO_SIZE_PER_SM) f->tx_state = FIFO_FULL;       
       else f->tx_state = FIFO_HAS_DATA;
       f->buffer[f->tx_bottom] = value;
   }
   set_status(f);
   PRINTI("writing %d, now in fifo: %d\n", value, f->tx_top - f->tx_bottom);    
   return true;
}

/************************************************************************************************************************
 *
 * read: pop top 
 * 
 ************************************************************************************************************************/

bool fifo_read(fifo_t * f, uint32_t * value_ptr) {
   if (f->mode == TX_ONLY) return false;
   if (f->rx_state == FIFO_EMPTY) return false;
   *value_ptr = f->buffer[f->rx_top-1];
   f->rx_top--;
   if (f->rx_top == 0) f->rx_state = FIFO_EMPTY;
   else f->rx_state = FIFO_HAS_DATA;
   set_status(f);
   PRINTI("reading %d, left in fifo: %d\n", *value_ptr, f->rx_top - f->rx_bottom);    
   return true;
}

/************************************************************************************************************************
 *
 * push: if room, put value on bottom of rx and shift everything up by one
 * 
 ************************************************************************************************************************/

bool fifo_push(fifo_t * f, uint32_t value) {
   int i;
   if (f->mode == TX_ONLY) return false;
   else { 
       if (f->rx_state == FIFO_FULL) return false;
       for (i = f->rx_top-1; i >= f->rx_bottom; i--) f->buffer[i+1] = f->buffer[i];
       f->rx_top = f->rx_top + 1;
       if (f->rx_top == f->tx_bottom) f->rx_state = FIFO_FULL;       
       else f->rx_state = FIFO_HAS_DATA;
       f->buffer[f->rx_bottom] = value;
   }
   set_status(f);
   PRINTI("pushed %d, now in fifo: %d\n", value, f->rx_top - f->rx_bottom);    
   return true;
}

/************************************************************************************************************************
 *
 * pull
 * 
 ************************************************************************************************************************/

bool fifo_pull(fifo_t * f, uint32_t * value_ptr) {
   if (f->mode == RX_ONLY) return false;
   if (f->tx_state == FIFO_EMPTY) return false;
   *value_ptr = f->buffer[f->tx_top-1];
   f->tx_top--;
   if (f->tx_top == f->tx_bottom) f->tx_state = FIFO_EMPTY;
   else f->tx_state = FIFO_HAS_DATA;
   set_status(f);
   PRINTI("pulled %d, now in fifo: %d\n", *value_ptr, f->tx_top - f->tx_bottom);    
   return true;
}

void fifo_copy(fifo_t * from, fifo_t * to) {
    int i;
    for (i=0; i<TOTAL_FIFO_SIZE_PER_SM; i++) to->buffer[i] = from->buffer[i];
    to->rx_state = from->rx_state;
    to->tx_state = from->tx_state;
    to->status = from->status;
    to->EXECCTRL_STATUS_SEL = from->EXECCTRL_STATUS_SEL;
    to->mode = from->mode;
    to->N = from->N;
    to->rx_bottom = from->rx_bottom;
    to->rx_top = from->rx_top;
    to->tx_bottom = from->rx_bottom;
    to->tx_top = from->tx_top;
}

fifo_compare_t fifo_compare(fifo_t * from, fifo_t * to) {
    int i;
    if (to->rx_state != from->rx_state) return RX_STATE;
    if (to->tx_state != from->tx_state) return TX_STATE;
    if (to->rx_bottom != from->rx_bottom) return RX_CONTENTS;
    if (to->rx_top != from->rx_top) return RX_CONTENTS;
    if (to->tx_bottom != from->rx_bottom) return TX_CONTENTS;
    if (to->tx_top != from->tx_top) return TX_CONTENTS;
    for (i=to->rx_bottom; i<to->rx_top; i++) {
        if (to->buffer[i] != from->buffer[i]) return RX_CONTENTS;
    }
    for (i=to->tx_bottom; i<to->tx_top; i++) {
        if (to->buffer[i] != from->buffer[i]) return TX_CONTENTS;
    }
    return FIFO_MATCH;
}

