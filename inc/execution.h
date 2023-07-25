/*!
 * @file /execution.h
 * @brief Simpio hardware simulation logic
 * @details
 * This is for controlling the execution of SMs and for interfacing with executing SMs (i.e., reading and writing input/outout data)
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef EXECUTION_H
#define EXECUTION_H

#include "hardware.h"

typedef enum {exec_normal, exec_interrupt, exec_idle } exec_context_e;

void exec_reset();

int8_t exec_first_instruction_that_will_be_executed();

int  exec_step_programs_next_instruction();  

int  exec_run_all_programs();         /* runs each defined program/SM in round robin fashion, one clock cycle each, until breakpoint */

bool exec_pio_read(uint8_t pio, uint8_t, uint8_t * value_read);

bool exec_pio_write(uint8_t pio, uint8_t, uint8_t value_to_write);

/* the following is used internally for excuting EXEC destination instructions; it might be useful for clients so including it just in case */
bool exec_instruction_decode(sm_t * sm);

#endif