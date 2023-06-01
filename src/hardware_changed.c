/*!
 * @file /hardware_changed.c
 * @brief HARDWARE STATE CHANGE TRACKING
 * @details
 * There are two fairly independent things in this file, perhaps should be separated.
 * 1) tracking changes (snapshots and comparing what changed)
 * 2) gpio history tracking (for timelines)
 *
 * This also tracks a predefined number of history values of GPIO pins to facilitate creating timelines.
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

  
#include "hardware_changed.h"
#include <stddef.h>
#include "ui.h"
#include "enumerator.h"

/***********************************************************************************************************
 * state data
 **********************************************************************************************************/

typedef struct {
    fifo_t   fifo;
    uint32_t scratch_x;
    uint32_t scratch_y;
    uint32_t osr;      
    uint32_t isr;      
    uint8_t  shift_out_count;
    uint8_t  shift_in_count;
    uint32_t exec_machine_instruction;
} sm_snapshot_t;

typedef struct {
    irq_t irqs[NUM_IRQS];
    bool  irqs_changed[NUM_IRQS];
} pio_snapshot_t;

static pio_snapshot_t pio_snapshots[NUM_PIOS];
static sm_snapshot_t  sm_snapshots[NUM_PIOS * NUM_SMS];
static gpio_t         gpio_snapshots[NUM_GPIOS];

hardware_changed_t hardware_changed;

/***********************************************************************************************************
 * snapshot
 **********************************************************************************************************/

#define SNAPSHOT(X) sm_snapshots[sm_num].X = sm->X; 

void hardware_snapshot() {
    int pio_num, sm_num, irq_num, gpio_num;
    bool gpio, gpio_dir;
    pio_num = 0;
    FOR_ENUMERATION(pio, pio_t, hardware_pio) {
      for (irq_num=0; irq_num < NUM_IRQS; irq_num++) pio_snapshots[pio_num].irqs[irq_num].value = pio->irqs[irq_num].value;
      pio_num++;
    }
    sm_num = 0;
    FOR_ENUMERATION(sm, sm_t, hardware_sm) {
      fifo_copy((&sm->fifo), &(sm_snapshots[sm_num].fifo));
      SNAPSHOT(scratch_x)
      SNAPSHOT(scratch_y)
      SNAPSHOT(osr)
      SNAPSHOT(isr)
      SNAPSHOT(shift_out_count)
      SNAPSHOT(shift_in_count)
      SNAPSHOT(exec_machine_instruction)
      sm_num++;
  }
  for (gpio_num=0; gpio_num<NUM_GPIOS; gpio_num++) {
      gpio = hardware_get_gpio(gpio_num);
      gpio_dir = hardware_get_gpio_dir(gpio_num);
      gpio_snapshots[gpio_num].value = gpio;
      gpio_snapshots[gpio_num].pindir = gpio_dir;
  }
}

/***********************************************************************************************************
 * compare what changed
 **********************************************************************************************************/

#define compare_set(XYZ) hardware_changed.sms[sm_num].XYZ = (sm_snapshots[sm_num].XYZ != sm->XYZ);

hardware_changed_t * hardware_get_changed() {
  int pio_num, sm_num, irq_num;
  bool gpio, gpio_dir;
  int gpio_num;
  pio_num = 0;
  FOR_ENUMERATION(pio, pio_t, hardware_pio) {
      for (irq_num=0; irq_num < NUM_IRQS; irq_num++) {
          hardware_changed.pios[pio_num].irqs[irq_num] = (pio->irqs[irq_num].value != pio_snapshots[pio_num].irqs[irq_num].value);
      }
      sm_num = 0;
      FOR_ENUMERATION(sm, sm_t, hardware_sm) {
        hardware_changed.sms[sm_num].fifo = fifo_compare(&(sm->fifo), &(sm_snapshots[sm_num].fifo));
        compare_set(scratch_x)
        compare_set(scratch_y)
        compare_set(osr)
        compare_set(isr)
        compare_set(shift_out_count)
        compare_set(shift_in_count)
        compare_set(exec_machine_instruction)
        sm_num++;
    }
  }
  for (gpio_num=0; gpio_num<NUM_GPIOS; gpio_num++) {
      gpio = hardware_get_gpio(gpio_num);
      gpio_dir = hardware_get_gpio_dir(gpio_num);
      hardware_changed.gpios[gpio_num].value = (gpio_snapshots[gpio_num].value != gpio);
      hardware_changed.gpios[gpio_num].pindir = (gpio_snapshots[gpio_num].pindir != gpio_dir);
  }
  return &hardware_changed;
}

/***********************************************************************************************************
 * timelines
 **********************************************************************************************************/

/* note: the values and ticks are in an array until max # reached and then the array becomes a circular buffer:
   
 (A) the oldest value is at current_index+1 which is the next value to be overwritten (*); 
     this should be at zero, current_index+2 should be at 1, current_index+3 should be at 2, etc up to max
     so requests for [0] should return current_index+1, requests for [1] should return current_index+2, up to requests for X which should return [max-1]
     or [current_index+1+i] for i=0; i<X; i++
     if (current_index+1+X = max) then X = max-current_index-1
     so return [current_index+1+i] for i=0; i<max-current_index-1; i++     
     
 (B) the most recent value is current_index, this should be at max-1, current_index-1 should be at max-2, etc.
     so requests for max-1 should return current_index, requests for max-2 should return current_index-1, etc. down to requests for X which should return [0]
     that is, requests for [max-1-i] should return [current_index-i]  for i=0; i<=current_index; i++; (note that current_index-current_index equals zero so that is the last one returned)
     or equivalently, requests for [i] should return [current_index+1-max+i] when i=max-current_index-1; i<max; i++; 
     (note that current_index+1-max+(max-current_index-1) is zero), the first one returned, and current_index+1-max+(max-1) = current_index which should be the last one returned, so this all works )
               
   since (A) and (B) together cover all mappings of [i] from i=0 to max-1, (A) and (B) are a complete mapping
   
   (*) note that if current_index+1 = max, then the oldest value is already at zero so no reordering is needed

*/

/***********************************************************************************************************
 * state data
 **********************************************************************************************************/

#define MAX_GPIO_HISTORY_INDEX 50

static gpio_history_t gpio_history[MAX_GPIO_HISTORY_INDEX];
static bool gpio_history_values[MAX_GPIO_HISTORY_INDEX][NUM_GPIOS];

static uint32_t gpio_history_count;              // how many values have been stored
static uint32_t gpio_history_current_index;      // index into the history for where the next value is to be stored
static uint32_t gpio_history_current_iterator;   // the index into the of the next value to return when iterating

/***********************************************************************************************************
 * functions
 **********************************************************************************************************/

uint32_t hardware_changed_gpio_history_init() { // returns the max number of values that can be stored in the history
    gpio_history_count = 0;
    gpio_history_current_index = 0;
    return MAX_GPIO_HISTORY_INDEX;
}
    
uint32_t hardware_changed_gpio_history_iteration() {  // returns the actual number of values that have been stored in the history
    gpio_history_current_iterator = 0;
    return gpio_history_count;
}

gpio_history_t * hardware_changed_gpio_history_get() { // returns next in history starting with oldest first; if called too many times then returns NULL until iteration called again
    uint32_t i = gpio_history_current_iterator++;
    uint32_t max = MAX_GPIO_HISTORY_INDEX;
    uint32_t current_index = gpio_history_current_index;
    if ( (gpio_history_count < MAX_GPIO_HISTORY_INDEX) || (current_index+1 == max) ) { // simple case where we haven't wrapped yet or we are just about to wrap
       return &(gpio_history[i]);
    }
    if (i < (max-current_index-1)) {  // case (A)
        return &(gpio_history[current_index + 1 + i]);
    }
    if (i < max) {   // case (B)
        return &(gpio_history[current_index+1-max+i]);
    }
    return NULL;
}


void hardware_changed_gpio_history_update() {
    uint8_t gpio;
    bool gpio_val;
    sm_t * sm;
    if (gpio_history_count < MAX_GPIO_HISTORY_INDEX) {  // have not yet wrapped
        gpio_history_count++;
        gpio_history_current_index++;
    }
    else { // wrapping
        if (gpio_history_current_index == MAX_GPIO_HISTORY_INDEX) gpio_history_current_index = 0;  // wrap
        else gpio_history_current_index++;
    }
    sm = hardware_sm_set();
    gpio_history[gpio_history_current_index].clock_tick = sm->clock_tick;
    for (gpio=0; gpio<NUM_GPIOS; gpio++) {
        gpio_val = hardware_get_gpio(gpio);
        gpio_history[gpio_history_current_index].values[gpio] = gpio_val;
    }
}


