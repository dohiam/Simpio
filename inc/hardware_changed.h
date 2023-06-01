/*!
 * @file /hardware_changed.h
 * @brief HARDWARE STATE CHANGE TRACKING
 * @details
 * The purpose of this module is to support highlighting when any piece of the hardware state changes.
 * Call hardware_snapshot to record a baseline, and then call hardware_get_changed to capture what changed since
 * the last captured baseline, and finally parse through the return value to see what specifically changed.
 *
 * This also tracks a predefined number of history values of GPIO pins to facilitate creating timelines.
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef HARDWARE_CHANGED_H
#define HARDWARE_CHANGED_H

#include "hardware.h"

typedef struct {
    fifo_compare_t fifo;
    bool scratch_x;
    bool scratch_y;
    bool osr;
    bool isr;
    bool shift_out_count;
    bool shift_in_count;
    bool exec;
    bool exec_machine_instruction;
} sm_changed_t;

typedef struct {
    bool  irqs[NUM_IRQS];
} pios_changed_t;

typedef struct {
  bool value;
  bool pindir; 
} gpios_changed_t;

typedef struct {
  pios_changed_t pios[NUM_PIOS];
  gpios_changed_t gpios[NUM_GPIOS];
  sm_changed_t sms[NUM_PIOS * NUM_SMS];
} hardware_changed_t;

void hardware_snapshot();

hardware_changed_t * hardware_get_changed();

// the following is for showing timelines - init, let collection happen, call iteration, then immediately call get right number of times or until NULL returned

typedef struct {
    bool  values[NUM_GPIOS];
    uint32_t clock_tick; 
} gpio_history_t;

void hardware_changed_gpio_history_update(); 

uint32_t hardware_changed_gpio_history_init(); // returns the max number of values that can be stored in the history

uint32_t hardware_changed_gpio_history_iteration();  // returns the actual number of values that have been stored in the history

gpio_history_t * hardware_changed_gpio_history_get(); // returns next in history starting with oldest first; if called too many times then returns NULL until iteration called again

#endif
