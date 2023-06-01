/*!
 * @file /fifo_test.c
 * @brief fifo test cases
 * @details
 * If nothing asserts then tests pass.
 *
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#include "fifo.h"
#include <stdio.h>
#include <assert.h>


fifo_t fifo; 
fifo_t * f = &fifo;

void dump(int from, int to) {
    int i;
    for (i=from; i<to; i++) printf("%d=%d  " , i, f->buffer[i]); printf("\n");
}


int main(int argc, char** argv) {
    
    int i;
    int* ip = &i;
    
    fifo_init(f, BIDI);
    assert (f->rx_state == FIFO_EMPTY);
    assert (f->tx_state == FIFO_EMPTY);
    assert (f->status);
    assert (f->mode == BIDI);
    
    assert(fifo_write(f, 1));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 2));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 3));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 4));  assert (f->tx_state == FIFO_FULL);
    assert(!fifo_write(f, 5));
    
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 1);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 2);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 3);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_EMPTY);
    assert (i == 4);
    assert(!fifo_pull(f, ip));
    
    assert(fifo_push(f, 1));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 2));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 3));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 4));  assert (f->rx_state == FIFO_FULL);
    assert(!fifo_push(f, 5));
    
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 1);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 2);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 3);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_EMPTY);
    assert (i == 4);
    assert(!fifo_read(f, ip));
    
    fifo_init(f, RX_ONLY);
    
    assert(!fifo_write(f,0));
    assert(fifo_push(f, 1));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 2));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 3));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 4));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 5));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 6));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 7));  assert (f->rx_state == FIFO_HAS_DATA);
    assert(fifo_push(f, 8));  assert (f->rx_state == FIFO_FULL);
    assert(!fifo_push(f, 9));
    
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 1);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 2);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 3);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 4);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 5);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 6);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_HAS_DATA);
    assert (i == 7);
    assert(fifo_read(f, ip));  assert (f->rx_state == FIFO_EMPTY);
    assert (i == 8);
    assert(!fifo_read(f, ip));
    
    fifo_init(f, TX_ONLY);
    
    assert(!fifo_read(f,ip));
    assert(fifo_write(f, 1));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 2));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 3));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 4));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 5));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 6));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 7));  assert (f->tx_state == FIFO_HAS_DATA);
    assert(fifo_write(f, 8));  assert (f->tx_state == FIFO_FULL);
    assert(!fifo_write(f, 9));
    
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 1);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 2);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 3);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 4);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 5);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 6);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_HAS_DATA);
    assert (i == 7);
    assert(fifo_pull(f, ip));  assert (f->tx_state == FIFO_EMPTY);
    assert (i == 8);
    assert(!fifo_pull(f, ip));
    
    fifo_init(f, BIDI);
    
    f->EXECCTRL_STATUS_SEL = false;  /* status is now about TX */
    f->N = 3;
    
    assert(fifo_write(f, 1));  assert (f->status);
    assert(fifo_write(f, 2));  assert (f->status);
    assert(fifo_write(f, 3));  assert (!f->status);
    assert(fifo_write(f, 4));  assert (!f->status);
    assert(fifo_pull(f, ip));  assert (!f->status);
    assert(fifo_pull(f, ip));  assert (f->status);
    assert(fifo_pull(f, ip));  assert (f->status);
    assert(fifo_pull(f, ip));  assert (f->status);
    
    f->EXECCTRL_STATUS_SEL = true;  /* status is now about RX */
    f->N = 3;

    assert(fifo_push(f, 1));  assert (f->status);
    assert(fifo_push(f, 2));  assert (f->status);
    assert(fifo_push(f, 3));  assert (!f->status);
    assert(fifo_push(f, 4));  assert (!f->status);
    assert(fifo_read(f, ip));  assert (!f->status);
    assert(fifo_read(f, ip));  assert (f->status);
    assert(fifo_read(f, ip));  assert (f->status);
    assert(fifo_read(f, ip));  assert (f->status);
    
    printf("Tests done; all pass.\n");
    
}