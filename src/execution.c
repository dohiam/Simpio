/*!
 * @file /execution.c
 * @brief Simpio hardware simulation logic
 * @details
 * Scheduling and execution of PIO instructions
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */


/**********************************************************************************
 * PIO_EXECUTION:
 * 
 * 1) Runs the round robin execution behavior of the simulator
 * 2) Defines the detailed behavior of each type of instruction
 * 
 **********************************************************************************/

#include "hardware.h"
#include "instruction.h"
#include "print.h"
#include "execution.h"
#include "hardware_changed.h"
#include "ui.h"

/***********************************************************************************************************
 * state data
 **********************************************************************************************************/

int current_definition;
int current_label;

program_t cp;  /* TODO multiple simultaneous program support */

/***********************************************************************************************************
 * helpers
 **********************************************************************************************************/

#define THIS_INSTRUCTION &(hardware_pio_set()->instructions[sm->pc])
#define THIS_SM hardware_sm_set()
#define INCREMENT_PC hardware_sm_set()->pc++
#define SHIFT_RIGHT(dir) dir

//get the i'th bit from a 32 bit value, starting at the LSB
bool ith_bit(uint8_t i, uint32_t from) {
    uint32_t bit_mask = ((uint32_t) 1) << i;
    uint32_t selected_bit = from & bit_mask;
    selected_bit = selected_bit >> i;
    if (selected_bit) return true;
    else return false;
}

//shift one bit into the target in the desired direction
void shift_into(bool direction, uint32_t * target, bool bit) {
    uint32_t masked;
    if (SHIFT_RIGHT(direction)) {
        masked = bit << 31;
        *target = (*target >> 1) + masked;
    }
    else {
        masked =  bit;
        *target = (*target << 1) + masked;
    }
}

//shift one bit into the target in the desired direction
void shift_into_16(bool direction, uint16_t * target, bool bit) {
    uint16_t masked;
    if (SHIFT_RIGHT(direction)) {
        masked = bit << 15;
        *target = (*target >> 1) + masked;
    }
    else {
        masked =  bit;
        *target = (*target << 1) + masked;
    }
}

// helper function for instruction decoding - gets (counting up from LSB to MSB) bits from ... to, shifted down so that the LSB of this bit range is the LSB of the returned result */
uint16_t instruction_field(uint16_t machine_instruction, uint16_t from, uint16_t to) {
    uint16_t i;
    uint16_t field = 0;
    for (i = from; i <= to; i++) {
        field += ith_bit(i, machine_instruction) << (i-from);   
    }
    return field;
}

/***********************************************************************************************************
 * instruction decode
 **********************************************************************************************************/

bool exec_instruction_decode(sm_t * sm) {
    uint8_t field;
    uint16_t mi = sm->exec_machine_instruction;
    PRINT("decoding %0X\n", mi);
    instruction_t * instr = &(sm->exec_instruction);
    instr->delay = instruction_field(mi, 8, (12 -sm->side_set_count));
    instr->side_set_value = instruction_field(mi, (12 - sm->side_set_count +1), 12); 
    uint8_t instruction_type = instruction_field(mi, 13, 15);
    instr->executing_sm = (void *) sm;
    instr->pio = sm->pio;
    instr->executing_sm_num = sm->this_num;

    switch (instruction_type) {
        case 0: /* JMP */
            instr->instruction_type = jmp_instruction;
            instr->side_set_value = -1;
            instr->jmp_pc_set = true;
            field = instruction_field(mi, 5, 7);  /* condition */
            switch (field) {
                case 0: /* always */
                    instr->condition = always;
                    break;
                case 1: /* x == 0 */
                    instr->condition = x_zero;
                    break;
                case 2: /* x-- != 0 */
                    instr->condition = x_decrement;
                    break;
                case 3: /* y == 0 */
                    instr->condition = y_zero;
                    break;
                case 4: /* y-- != 0 */
                    instr->condition = y_decrement;
                    break;
                case 5: /* x != y */
                    instr->condition = x_not_equal_y;
                    break;
                case 6: /* pin */
                    instr->condition = pin_condition;
                    break;
                case 7: /* !OSRE */
                    instr->condition = not_osre;
                    break;
                default:
                    instr->condition = unset_condition;
            };
            instr->jmp_pc = instruction_field(mi, 0, 4)-1;
            break;
        case 1: /* WAIT */
            instr->instruction_type = wait_instruction;
            instr->polarity = instruction_field(mi, 7, 7);
            instr->index_or_value = instruction_field(mi, 0, 4);
            field = instruction_field(mi, 5, 6);  /* source */
            switch(field) {
                case 0:
                    instr->wait_source = gpio_source;
                    break;
                case 1:
                    instr->wait_source = pin_source;
                    break;
                case 2:
                    instr->wait_source = irq_source;
                    break;
                case 3:
                    instr->wait_source = reserved_wait_source;
                    break;
                default:
                    instr->wait_source = unset_wait_source;
            };
            break;
        case 2: /* IN */
            instr->instruction_type = in_instruction;
            instr->bit_count = instruction_field(mi, 0, 4);
            field = instruction_field(mi, 5, 7);
            switch (field) {
                case 0:
                    instr->source = pins_source;
                    break;
                case 1:
                    instr->source = x_source;
                    break;
                case 2:
                    instr->source = y_source;
                    break;
                case 3:
                    instr->source = null_source;
                    break;
                case 4:
                    instr->source = reserved_source;
                    break;
                case 5:
                    instr->source = reserved_source;
                    break;
                case 6:
                    instr->source = isr_source;
                    break;
                case 7:
                    instr->source = osr_source;
                    break;
                default:
                    instr->source = unset_source;
                    break;
            };
            break;
        case 3: /* OUT */
            instr->instruction_type = out_instruction;
            instr->bit_count = instruction_field(mi, 0, 4);
            field = instruction_field(mi, 5, 7);
            switch (field) {
                case 0:
                    instr->destination = pins_destination;
                    break;
                case 1:
                    instr->destination = x_destination;
                    break;
                case 2:
                    instr->destination = y_destination;
                    break;
                case 3:
                    instr->destination = null_destination;
                    break;
                case 4:
                    instr->destination = pindirs_destination;
                    break;
                case 5:
                    instr->destination = pc_destination;
                    break;
                case 6:
                    instr->destination = isr_destination;
                    break;
                case 7:
                    instr->destination = exec_destination;
                    break;
                default:
                    instr->destination = unset_destination;
                    break;
            };
            break;
        case 4: /* PUSH & PULL */
            instr->block = instruction_field(mi, 5, 5);
            field = instruction_field(mi, 7, 7);
            if (field) {
                instr->instruction_type = pull_instruction;
                instr->if_empty = instruction_field(mi, 6, 6);
            }
            else {
                instr->instruction_type = push_instruction;
                instr->if_full = instruction_field(mi, 6, 6);
            }
            break;
        case 5: /* MOV */
            instr->instruction_type = mov_instruction;
            field = instruction_field(mi, 5, 7);
            switch (field) {
                case 0:
                    instr->destination = pins_destination;
                    break;
                case 1:
                    instr->destination = x_destination;
                    break;
                case 2:
                    instr->destination = y_destination;
                    break;
                case 3:
                    instr->destination = reserved_destination;
                    break;
                case 4:
                    instr->destination = exec_destination;
                    break;
                case 5:
                    instr->destination = pc_destination;
                    break;
                case 6:
                    instr->destination = isr_destination;
                    break;
                case 7:
                    instr->destination = osr_destination;
                    break;
                default:
                    instr->destination = unset_destination;
                    break;
            };
            field = instruction_field(mi, 0, 2);
            switch (field) {
                case 0:
                    instr->source = pins_source;
                    break;
                case 1:
                    instr->source = x_source;
                    break;
                case 2:
                    instr->source = y_source;
                    break;
                case 3:
                    instr->source = null_source;
                    break;
                case 4:
                    instr->source = reserved_source;
                    break;
                case 5:
                    instr->source = status_source;
                    break;
                case 6:
                    instr->source = isr_source;
                    break;
                case 7:
                    instr->source = osr_source;
                    break;
                default:
                    instr->source = unset_source;
                    break;
            };
            field = instruction_field(mi, 3, 4);
            switch (field) {
                case 0:
                    instr->operation = no_operation;
                    break;
                case 1:
                    instr->source = invert;
                    break;
                case 2:
                    instr->source = bit_reverse;
                    break;
                case 3:
                    instr->source = reserved_operation;
                    break;
                default:
                    instr->source = unset_operation;
                    break;
            };
            break;
        case 6: /* IRQ */
            instr->instruction_type = irq_instruction;
            instr->index_or_value = instruction_field(mi, 0, 4);
            instr->wait = instruction_field(mi, 5, 5);
            instr->clear = instruction_field(mi, 6, 6);
            break;
        case 7: /* SET */
            instr->instruction_type = set_instruction;
            instr->index_or_value = instruction_field(mi, 0, 4);
            field = instruction_field(mi, 5, 7);
            switch (field) {
                case 0:
                    instr->destination = pins_destination;
                    break;
                case 1:
                    instr->destination = x_destination;
                    break;
                case 2:
                    instr->destination = y_destination;
                    break;
                case 3:
                    instr->destination = reserved_destination;
                    break;
                case 4:
                    instr->destination = pindirs_destination;
                    break;
                case 5:
                case 6:
                case 7:
                    instr->destination = reserved_destination;
                    break;
                default:
                    instr->destination = unset_destination;
                    break;
            };
            
            break;
        default:
            PRINT("Unexpected instruction to decode %d\n", instruction_type);
            
    };
}

/***********************************************************************************************************
 * find instructions for scheduling
 **********************************************************************************************************/

bool exec_run_instruction(instruction_t * instruction);
bool exec_run_user_instruction(user_instruction_t * instruction);

instruction_t* next_instruction() {
    static sm_t  *sm;
    static hardware_sm_enumerator_t e;
    int sm_count;
    pio_t * pio;
    bool found_sm_with_instructions;
    instruction_t* found_instruction;
    /* get next instruction, round robin-style across all pios and sms and user_processors
       skipping any/all sms with negative pc (i.e., without any instructions to execute),
       and guarding against infinitely cycling through everything */
    for (sm_count = 0, found_sm_with_instructions = false; sm_count < (NUM_PIOS * NUM_SMS) && !found_sm_with_instructions; sm_count++) {
      if (!sm) {  /* this should only be true when first initialized */
        sm = hardware_sm_first(&e);
      }
      else {  /* not the first time through */
        sm = hardware_sm_next(&e);
        if (!sm) {
          sm = hardware_sm_first(&e);
        }
      }
      pio = (pio_t *) sm->pio;
      if ( (sm->pc >= 0) && (pio->instructions[sm->pc].instruction_type != empty_instruction) ) {
          found_sm_with_instructions = true;
          found_instruction = &(pio->instructions[sm->pc]);
          found_instruction->executing_sm = (void *) sm;
      }
    }
    if (found_sm_with_instructions) {
        return found_instruction;
    }
    else return NULL;
}

// this is just to get the first instruction that will be executed to update UI when first stepping
int8_t first_instruction_line() {
    pio_t *pio;
    FOR_ENUMERATION(sm, sm_t, hardware_sm) {
        pio = (pio_t *) sm->pio;
        if ( (sm->pc >= 0) && (pio->instructions[sm->pc].instruction_type != empty_instruction) ) {
                return pio->instructions[sm->pc].line;
        }
    }
    return -1;
}

user_instruction_t * next_user_instruction(bool dont_switch) {
    static user_processor_t *up = NULL;
    static hardware_user_processor_enumerator_t e;
    bool found_up_with_instructions;
    int up_count;
    user_instruction_t * found_instruction;
    /* get next instruction, round robin-style across all user_processors, 
       skipping any/all ups with negative pc (i.e., without any instructions to execute),
       and guarding against infinitely cycling through everything */
    for (up_count = 0, found_up_with_instructions = false; up_count < (NUM_USER_PROCESSORS) && !found_up_with_instructions; up_count++) {
      if (!up) {  /* this should only be true when first initialized */
        up = hardware_user_processor_first(&e);
      }
      else {  /* not the first time through */
        if (!dont_switch) up = hardware_user_processor_next(&e);
        if (!up) {
          continue;
        }
      }
      if ( (up->pc >= 0) && (up->instructions[up->pc].instruction_type != empty_user_instruction) ) {
          found_up_with_instructions = true;
          found_instruction = &(up->instructions[up->pc]);
          found_instruction->executing_up = (void *) up;
      }
    }
    if (found_up_with_instructions) {
        return found_instruction;
    }
    else { 
        return NULL;
    }
}

// this is just to get the first user instruction that will be executed to update UI when first stepping
int8_t first_user_instruction_line() {
    FOR_ENUMERATION(up, user_processor_t, hardware_user_processor) {
        if (up->pc >= 0) return up->instructions[up->pc].line;
    }
    return -1;
}

int8_t exec_first_instruction_that_will_be_executed() {
    int first_line;
    first_line = first_user_instruction_line();
    if (first_line >= 0) return first_line;
    else return first_instruction_line();
}

/* helper routines
 *   if there is already an instruction of the right type, we're good to go, 
 *   else look for an instruction of the right type
 */
bool try_user(user_instruction_t ** instr) {
    bool found_instruction;
    if (*instr == NULL) {
        *instr = next_user_instruction(false);
        if (*instr == NULL) {
            found_instruction = false;
        }
        else found_instruction = true;
    }
    else found_instruction = true;
    return found_instruction;
}
bool try_sm(instruction_t ** instr) {
    bool found_instruction;
    if (*instr == NULL) {
        *instr = next_instruction();
        if (*instr == NULL) {
            found_instruction = false;
        }
        else found_instruction = true;
    }
    else found_instruction = true;
    return found_instruction;
}
 
/***********************************************************************************************************
 * execution 
 **********************************************************************************************************/

int exec_step_programs_next_instruction() {
    static user_instruction_t* user_instruction = NULL;
    static instruction_t* instruction = NULL;
    static bool try_user_first = true;
    static bool found_user_instruction;
    static bool found_sm_instruction;
    sm_t * sm;
    bool completed;
    int8_t next_line;
    
    found_user_instruction = try_user(&user_instruction);
    found_sm_instruction = try_sm(&instruction);
    
    if (!found_user_instruction && !found_sm_instruction) 
        return -1;
    
    if ( (try_user_first && found_user_instruction) || (!try_user_first  && !found_sm_instruction) ) {
        // execute user instruction and get next one
        PRINTD("Trying UP first: delay:%d delay_left:%d continue:%d\n", user_instruction->delay, user_instruction->delay_left, user_instruction->continue_user); 
        if (user_instruction->continue_user && (user_instruction->delay == 0 || user_instruction->delay_left == 1)) { 
            status_msg("to continue to next user instruction\n"); 
            try_user_first = true; 
        }
        else try_user_first = false;
        completed = exec_run_user_instruction(user_instruction);
        user_instruction = next_user_instruction(try_user_first);  /*dont_switch user processors if in continue_state */
        found_user_instruction = try_user(&user_instruction);
        //now find the line of the next instruction that will execute next time around
        if (!try_user_first) {
            if (found_sm_instruction) return instruction->line;
            if (found_user_instruction) return user_instruction->line;
        }
        else {
            if (found_user_instruction) return user_instruction->line;
            if (found_sm_instruction) return instruction->line;
        }
        return -1;
    }

    if ( (!try_user_first && found_sm_instruction) || (try_user_first  && !found_user_instruction) ) {
        // execute instruction and get next one
        PRINTD("Trying SM first: delay:%d delay_left:%d\n", instruction->delay, instruction->delay_left); 
        try_user_first = true;
        completed = exec_run_instruction(instruction);        
        sm = (sm_t *) instruction->executing_sm;
        sm->clock_tick++;
        instruction = next_instruction();
        found_sm_instruction = try_sm(&instruction);
        if (found_user_instruction) return user_instruction->line;
        if (found_sm_instruction) return instruction->line;
        return -1;
    }
    
    // should never get here, but ...
    return -1;
}
 
/* runs each defined program/SM in round robin fashion, one clock cycle each, until breakpoint (note will always run at least one instruction) */
int exec_run_all_programs() {
    bool hit_breakpoint = false;
    bool hit_break_key = false;
    int next_line;
    instruction_t * next_instruction;
    user_instruction_t * next_user_instruction;
    ui_enter_run_break_mode();
    while (!hit_breakpoint && !hit_break_key) {
      next_line = exec_step_programs_next_instruction();
      hit_breakpoint = instruction_is_breakpoint(next_line);
      if (!hit_breakpoint) {
          next_line = exec_step_programs_next_instruction();
          hit_breakpoint = instruction_is_breakpoint(next_line);
      }
      hit_break_key = ui_break_check();
    }
    ui_exit_run_break_mode();
    return next_line;
}


/************************************************************************************************************************
 * execution for each instruction
 ***********************************************************************************************************************/

/********************************
 **** JMP Instruction ***********
 *******************************/

bool run_jmp_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool branch = false;
    switch(instruction->condition) {
        case always:
        case unset_condition:
            branch = true;
            break;
        case x_zero:
            if (sm->scratch_x == 0) branch = true;
            break;
        case y_zero:
            if (sm->scratch_y == 0) branch = true;
            break;
        case x_decrement:
            if (--(sm->scratch_x) != 0) branch = true;
            break;
        case y_decrement:
            if (--(sm->scratch_y) != 0) branch = true;
            break;
        case x_not_equal_y:
            if (sm->scratch_x != sm->scratch_y) branch = true;
            break;
        case pin_condition:
            PRINTD("pin condition %d = %d \n", sm->pin_condition,hardware_get_gpio(sm->pin_condition));
            if (0 <= sm->pin_condition && sm->pin_condition <= 31 && hardware_get_gpio(sm->pin_condition)) branch = true;
            break;
        case not_osre:
            /* not true if bits shifted since the last pull does not yet meet the configured pull shift threshold */
            if (0 <= sm->shiftctl_pull_thresh && sm->shiftctl_pull_thresh <= 31 && sm->shift_out_count < sm->shiftctl_pull_thresh) branch = true;
            break;
    };
    if (!instruction->jmp_pc_set) {
        if (branch) {
            instruction->jmp_pc = instruction_label_location(instruction->location);  /* uses location into labels array to find instruction location */
            PRINTD("Jumping to instruction %d (line %d)\n", instruction->jmp_pc, instruction->location);
        }
        else {
            instruction->jmp_pc = sm->pc + 1;
            PRINTD("Continuing to instruction %d\n", instruction->jmp_pc);
        }
    }
    return true;
}

/********************************
 **** WAIT Instruction **********
 *******************************/

bool run_wait_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed;
    int pin_index, pin_base, polarity, irq_index, pin_value, irq_value;
    switch(instruction->wait_source) {
        case gpio_source:
            // wait until the gpio indicated by the index part of the instruction matches the polarity
            pin_index = instruction->index_or_value;
            pin_value = hardware_get_gpio(pin_index);
            polarity = instruction->polarity ;
            PRINTD("waiting on gpio: %d, polarity: %d, pin is now %d\n", pin_index, polarity, pin_value);
            if (pin_value == polarity) completed = true;
            else completed = false;
            break;
        case pin_source:
            // same as above except that the index from the instruction is added to the SM's *IN* pin_base (mod 32)
            pin_index = instruction->index_or_value;
            pin_base = sm->in_pins_base;
            pin_index = (pin_index + pin_base) % 32;
            pin_value = hardware_get_gpio(pin_index);
            polarity = instruction->polarity;
            PRINTD("waiting on pin: %d, polarity: %d, pin is now %d\n", pin_index, polarity, pin_value);
            if (pin_value == polarity) completed = true;
            else completed = false;
            break;
        case irq_source:
            // same as pin except it is the irq selected by index and if irq is cleared if it is 1 and the wait condition (polarity) is 1
            irq_index = instruction->index_or_value;
            polarity = instruction->polarity; 
            irq_value = hardware_irq_flag_is_set(irq_index);
            PRINTD("waiting on irq: %d, polarity: %d, pin is now %d\n", irq_index, polarity, irq_value);
            completed = false;
            if (polarity && irq_value) completed = true;
            if (!polarity && !irq_value) completed = true;
            break;
        case unset_wait_source:
            PRINT("Error: wait statement on line %d ignored because wait source is not set\n", instruction->line);
            break;
    };
    PRINTI("waiting done: %d\n", completed);
    return completed;
}

/********************************
 **** NOP Instruction ***********
 *******************************/

bool run_nop_instruction(instruction_t * instruction) {
    PRINTI("nop instruction\n");
    return true;
}

/********************************
 **** PUSH Instruction **********
 *******************************/

bool run_push_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool only_if_shifting_complete = instruction->if_full;  /* as I understand the meaning of iffull, calling only_if_shifting_complete is a better name for it and won't be confused with the FIFO full condition*/
    bool block = instruction->block;
    int  thresh = sm->shiftctl_push_thresh;
    PRINTD("iffull:%d; block:%d, thresh:%d, count:%d\n", only_if_shifting_complete, block, thresh, sm->shift_in_count);
    bool try_to_push;
    if (only_if_shifting_complete) { 
        if (sm->shift_in_count < thresh) {
            PRINTI("push skipped because shifting not complete\n");
            return true;  /* instruction completed without doing anything */
        }
    }
    if (block && (sm->fifo.rx_state == FIFO_FULL)) {
        PRINTI("push blocked because fifo is full\n");
        return false;  /* simulate the block by returning that this instruction wasn't completed */
    }
    if (!block && (sm->fifo.rx_state == FIFO_FULL)) {
        PRINTI("push skipped because fifo is full\n");
        return true; /* don't block but don't push */
    }
    /* if we didn't block or return without doing anything, then actually do the push */
    fifo_push(&(sm->fifo), sm->isr);
    sm->isr = 0;
    sm->shift_in_count = 0;
    return true;
}

bool run_autopush(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    int push_threshold = sm->shiftctl_push_thresh;
    PRINTD("trying autopush, threshold:%d; count:%d, fifo state: %d\n", push_threshold, sm->shift_in_count, sm->fifo.rx_state);
    if (sm->shift_in_count < push_threshold) return true; /* completed but no push, not ready for it yet */
    if (sm->fifo.rx_state == FIFO_FULL) return false; /* simulate the block/stall */
    /* if we didn't block or return without doing anything, the actually do the push */
    fifo_push(&(sm->fifo), sm->isr);
    sm->isr = 0;
    sm->shift_in_count = 0;
    return true;
}


/********************************
 **** IN Instruction ************
 *******************************/

bool run_in_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed, stalled;
    int cntr, bits_todo, gpio;
    bool bit_to_shift;
    int shift_threshold = sm->shiftctl_push_thresh; 
    int shift_direction = sm->shiftctl_in_shiftdir; 
    /* if there was an autopush that stalled previously, then we couldn't complete previously so we need to resume shifting now instead of starting anew */
    if (sm->shift_in_resume_count > 0) bits_todo = sm->shift_in_resume_count;
    else {
        if (instruction->source == pins_source) bits_todo = instruction->bit_count;
        else bits_todo = instruction->bit_count;
    }
    stalled = false;
    PRINTI("shifting in: ");
    for (cntr = 0; cntr < bits_todo && !stalled; cntr++) {
        switch (instruction->source) {
            case pins_source:
                gpio = (sm->in_pins_base + cntr) % 32;
                bit_to_shift = hardware_get_gpio(gpio);
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                break;
            case x_source:
                bit_to_shift = ith_bit(cntr, sm->scratch_x);
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                break;
            case y_source:
                bit_to_shift = ith_bit(cntr, sm->scratch_y);
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                break;
            case null_source:
                bit_to_shift = 0;
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                break;
            case isr_source:
                bit_to_shift = ith_bit(cntr, sm->isr);
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                break;
            case osr_source:
                bit_to_shift = ith_bit(cntr, sm->osr);
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                break;
            default: PRINT("Error: invalid source for IN instruction %d, line %d\n", instruction->source, instruction->line);
        };
        PRINTI("%d", bit_to_shift); 
        sm->shift_in_count++;
        if (sm->autopush && shift_threshold > 0 && sm->shift_in_count >= shift_threshold) {
            completed = run_autopush(instruction);
            stalled = !completed;
            if (stalled) sm->shift_in_resume_count = cntr; // the push stalled so we can't complete shifting and will need to resume later
            else sm->shift_in_resume_count = 0;
        }
        else completed = true;
    }
    PRINTI("\n"); 
    return completed;
}

/********************************
 **** PULL Instruction **********
 *******************************/

bool run_pull_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool only_if_shifting_complete = instruction->if_empty;
    bool block = instruction->block;
    bool try_to_pull;
    if (only_if_shifting_complete) { 
        int pull_threshold = sm->shiftctl_pull_thresh;   /* instruction_find_definition("SHIFTCTRL_PULL_THRESH", 32); */
        if (sm->shift_out_count < pull_threshold) return true;  /* instruction completed without doing anything */
    }
    if (sm->fifo.tx_state == FIFO_EMPTY) {
        if (block) {
            PRINTI("pull blocking, nothing in FIFO to pull\n");
            return false;  /* simulate the block by returning that this instruction wasn't completed */
        }
        else {
            PRINTI("pulling X because FIFO empty and nonblocking\n");
            sm->osr = sm->scratch_x;
            sm->shift_out_count = 0;
            sm->shift_out_resume_count = 0;
            return true;  /* simulate the block by returning that this instruction wasn't completed */
        }
    }
    /* if we didn't block or return without doing anything, the actually do the pull */
    fifo_pull(&(sm->fifo), &(sm->osr));
    PRINTI("Pulled %d from FIFO into OSR\n", sm->osr);
    sm->shift_out_count = 0;
    sm->shift_out_resume_count = 0;
    return true;
}

bool run_autopull(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    int pull_threshold = sm->shiftctl_pull_thresh;
    if (sm->shift_out_count < pull_threshold) return true; /* completed but no push, not ready for it yet */
    if (sm->fifo.tx_state == FIFO_EMPTY) return false; /* simulate the block/stall */
    /* if we didn't block or return without doing anything, the actually do the pull */
    fifo_pull(&(sm->fifo), &(sm->osr));
    sm->shift_out_count = 0;
    return true;
}

/********************************
 **** OUT Instruction ***********
 *******************************/

bool run_out_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed, stalled;
    uint8_t cntr, bits_todo;
    int gpio;
    bool bit_to_shift;
    int shift_threshold = sm->shiftctl_pull_thresh; 
    int shift_direction = sm->shiftctl_out_shiftdir; 
    /* if there was an autopull that stalled previously, then we couldn't complete previously so we need to resume shifting now instead of starting anew */
    if (sm->shift_out_resume_count > 0) bits_todo = sm->shift_out_resume_count;
    else {
        if (instruction->source == pins_source) bits_todo = (sm->out_pins_num > instruction->bit_count) ? instruction->bit_count : sm->out_pins_num;
        else bits_todo = instruction->bit_count;
    }
    if ( (shift_threshold > 0 && sm->shift_out_count >= shift_threshold) || (sm->shift_out_count >= 32) ) stalled = true;
    else stalled = false;
    for (cntr = 0; cntr < bits_todo && !stalled; cntr++) {
        if (SHIFT_RIGHT(shift_direction)) bit_to_shift = ith_bit(sm->shift_out_count, sm->osr);
        else bit_to_shift = ith_bit(shift_threshold - sm->shift_out_count - 1, sm->osr);
        switch (instruction->destination) {
            case pins_destination:
                gpio = (sm->out_pins_base + sm->shift_out_count) % 32;
                hardware_set_gpio(gpio , bit_to_shift);
                PRINTI("shifted out: %d to gpio:%d\n", bit_to_shift, gpio);
                break;
            case x_destination:
                shift_into(shift_direction, &(sm->scratch_x), bit_to_shift);
                PRINTI("shifted out: %d to x:%X\n", bit_to_shift, sm->scratch_x);
                break;
            case y_destination:
                shift_into(shift_direction, &(sm->scratch_y), bit_to_shift);
                PRINTI("shifted out: %d to y:%X\n", bit_to_shift, sm->scratch_y);
                break;
            case null_destination:
                sm->shift_out_count++;  /* discarding what would be shifted */
                PRINTI("shifting out to null\n");
                break;
            case pindirs_destination:
                // write this bit onto the gpios using the right direction, updating the count
                gpio = (sm->in_pins_base + bits_todo - cntr) % 32;
                if (shift_direction)  hardware_set_gpio_dir(gpio, bit_to_shift);
                else hardware_set_gpio_dir(gpio, bit_to_shift);
                PRINTI("shifted out: %d to gpio direction:%d\n", bit_to_shift, gpio);
                break;
            case pc_destination:
                shift_into(shift_direction, &(sm->pc_temp), bit_to_shift);
                PRINTI("shifted out: %d to PC:%d\n", bit_to_shift, sm->pc_temp);
                break;
            case isr_destination:
                shift_into(shift_direction, &(sm->isr), bit_to_shift);
                PRINTI("shifted out: %d to y:%X\n", bit_to_shift, sm->isr);
                sm->shift_in_count = cntr;
                break;
            case exec_destination:
                shift_into_16(shift_direction, &(sm->exec_machine_instruction), bit_to_shift);
                PRINTI("shifted out: %d to y:%X\n", bit_to_shift, sm->exec_machine_instruction);
                break;
            default: PRINT("Error: invalid source for IN instruction %d, line %d\n", instruction->source, instruction->line);
        };
        sm->shift_out_count++;
        if ( (shift_threshold > 0 && sm->shift_out_count >= shift_threshold) || (sm->shift_out_count >= 32) ) stalled = true;
        else stalled = false;
    }
    //if need to autopull
    if (sm->autopull && stalled)  {
        PRINTI("Performing Auto Pull\n");
        completed = run_autopull(instruction);
        if (completed) sm->shift_out_resume_count = 0;
    }
    else {
        completed = true;
        sm->shift_out_resume_count = 0;
    }
    if (completed && instruction->destination == pc_destination) {
        pio_t * pio = (pio_t *) sm->pio;
        if (0 <= sm->pc_temp && sm->pc_temp < pio->next_instruction_location) {
          PRINTI("setting PC to %d\n", sm->pc_temp);
          sm->pc = sm->pc_temp-1;  /* one will be added to the pc as part of general intruction execution */
        }
        else PRINTI("No instruction at PC %d, ignoring\n", sm->pc_temp);
    }
    if (completed && instruction->destination == exec_destination) {
        PRINTD("to execute %04X\n", sm->exec_machine_instruction);
        completed = false;
        if (exec_instruction_decode(sm)) {
            sm->exec_instruction.executing_sm = (void *) sm;
            print_instruction(&(sm->exec_instruction));
            while (!completed) {
                PRINTI("running decoded instruction\n");   
                completed = exec_run_instruction(&(sm->exec_instruction));
                PRINTI("decoded instruction run, completed=%d\n", completed);   
            }
        }
        else PRINT("Error: EXEC instruction did not decode properly\n");
    }
    return completed;
}

uint32_t op_bit_reverse(uint32_t input) {
    uint32_t output = 0;
    uint32_t mask = 1;
    int i;
    // shift in top bit to the bottom
    for (i=0; i<32; i++) {
        output <<= 1;
        output += input & mask;
        input >>= 1;
    }
    return output;
}

/********************************
 **** MOV Instruction ***********
 *******************************/

bool run_mov_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed = true;
    uint8_t pin_value;
    int pin_num;
    uint32_t value;
    switch(instruction->source) {
        case pins_source: 
            value = 0;
            // for highest numbered pin downto base pin, set lsb to pin value, shifting up to make room for each pin value added
            PRINTI("moving from pins: ");
            pin_num = 32; /* as per datasheet, always reads 32 consecutive pins, wrapping at pin 31 */
            int pin_count = 0;
            for (pin_num = sm->in_pins_base; pin_count < 32; pin_count++) {
                pin_value = hardware_get_gpio(pin_num);
                PRINTI("%d", pin_value);
                value = value << 1;
                value += pin_value;
                if (pin_num == 31) pin_num = 0;
                else pin_num = pin_num + 1;
            }
            PRINTI(" ");
            break;
        case x_source: 
            value = sm->scratch_x;
            PRINTI("moving from x: %X ", value);
            break;
        case y_source:
            value = sm->scratch_y;
            PRINTI("moving from y: %X ", value);
            break;
        case null_source:
            value = 0;
            PRINTI("moving from null: 0 ");
            break;
        case status_source:
            value = sm->fifo.status;
            PRINTI("moving from x: %X ", value);
            break;
        case isr_source: 
            value = sm->isr;
            PRINTI("moving from isr: %X ", value);
            break;
        case osr_source: 
            value = sm->osr;
            PRINTI("moving from osr: %X ", value);
            break;
        default: 
            PRINT("ERROR: invalid source for mov instruction\n");
    };
    // perform operation on value
    switch(instruction->operation) {
         case no_operation:
             break;
         case invert:
             PRINTI("inverting value\n");
             value = ~value;
             break;
         case bit_reverse:
             PRINTI("reversing value\n");
             value = op_bit_reverse(value);
             break;
         default:
             break;
    };
    //now send value to the right destination
    switch(instruction->destination) {
        case pins_destination: 
            PRINTI("value: %X to pins: ", value);
            for (pin_num = sm->out_pins_base; pin_num < (sm->out_pins_base + sm->out_pins_num); pin_num++) {
                hardware_set_gpio(pin_num, value %  2);
                PRINTI("%d", value %  2);
                value = value >> 1;
            }
            PRINTI("\n");
            break;
        case x_destination: 
            sm->scratch_x = value;
            PRINTI("to x\n");
            break;
        case y_destination:
            sm->scratch_y = value;
            PRINTI("to y\n");
            break;
        case reserved_destination:
            break;
        case exec_destination:
            sm->exec_machine_instruction = value;
            exec_instruction_decode(sm);
            print_instruction(&(sm->exec_instruction));
            PRINTI("now running EXEC instruction\n");
            exec_run_instruction(&(sm->exec_instruction));
            break;
        case pc_destination:
            sm->pc = value-1;  /* account for the increment that will happen when this instruction completes */
            PRINTI("to pc\n");
            break;
        case isr_destination:
            sm->isr = value;
            PRINTI("to isr\n");
            break;
        case osr_destination:
            sm->osr = value;
            PRINTI("to osr\n");
            break;
        default: 
            PRINT("ERROR: invalid destination for set instruction: ");
    };
    return completed;
}

/********************************
 **** SET Instruction **********
 *******************************/

bool run_set_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    int pin_num;
    uint32_t value = instruction->index_or_value;
    switch(instruction->destination) {
        case pins_destination: 
            PRINTI("setting pins %d..%d to %d\n", sm->set_pins_base, sm->set_pins_base + sm->set_pins_num, value);
            for (pin_num = sm->set_pins_base; pin_num < (sm->set_pins_base + sm->set_pins_num); pin_num++) {
                hardware_set_gpio(pin_num, value %  2);
                value = value >> 1;
            }
            break;
        case x_destination: 
            PRINTI("setting x to %0X\n", value);
            sm->scratch_x = value;
            break;
        case y_destination:
            PRINTI("setting y to %0X\n", value);
            sm->scratch_y = value;
            break;
        case pindirs_destination: 
            PRINTI("setting pin %d to direction to %d\n", pin_num, value);
            hardware_set_gpio_dir(pin_num, value);
            break;
        default: 
            PRINT("ERROR: invalid destination for set instruction: ");
    };
    return true; /* set instruction never blocks */
}

/********************************
 **** IRQ Instruction **********
 *******************************/

bool run_irq_instruction(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed = true;
    switch (instruction->operation) {
        case set_operation:
            PRINTI("setting irq %d\n", instruction->index_or_value);
            hardware_irq_flag_set(instruction->index_or_value, true);
            break;
        case clear_operation:
            PRINTI("clearing irq %d\n", instruction->index_or_value);
            hardware_irq_flag_set(instruction->index_or_value, false);
            break;
        default:
            PRINT("invalid irq operation, line %d\n", instruction->line);
    };
    /* TODO */
    return completed;
}

/********************************
 **** WRITE Instruction *********
 *******************************/

/* write is a meta instruction to simulate a client C program writing to this output device */
bool run_write_instruction(user_instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed;
    if (sm->fifo.tx_state != FIFO_FULL) {
        fifo_write(&(sm->fifo), instruction->value);
        completed = true;
    }
    else {
        PRINTD("write can't be done because TX FIFO is full\n");
        completed = false;
    }
    return completed;
}

/********************************
 **** READ Instruction *********
 *******************************/

/* read is a meta instruction to simulate a client C program writing to this output device */
bool run_read_instruction(user_instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    bool completed;
    bool rc;
    uint32_t value;
    if (sm->fifo.rx_state != FIFO_EMPTY) {
        fifo_read(&(sm->fifo), &value);
        rc = instruction_var_set(instruction->var_name, value);
        if (!rc) { PRINT("unable to set %s to %d\n", instruction->var_name, value); }
        completed = true;
    }
    else completed = false;
    return completed;
}

/********************************
 **** PIN Instruction *********
 *******************************/

/* pin is a meta instruction to simulate a client C program writing to this output device */
bool run_pin_instruction(user_instruction_t * instr) {
    sm_t * sm = (sm_t *) instr->executing_sm;
    bool completed;
    bool rc;
    PRINTI("setting pin %d to %d\n", instr->pin, instr->set_high);
    if (instr->pin >= 0) hardware_set_gpio(instr->pin, instr->set_high);
    else PRINT("unable to set pin %d\n", instr->pin);
    return true;
}

void run_side_set(instruction_t * instruction) {
    sm_t * sm = (sm_t *) instruction->executing_sm;
    int pin_num;
    uint32_t value = instruction->side_set_value;
    PRINTI("running side set: base=%d num=%d value=%d\n", sm->side_set_pins_base, sm->side_set_pins_num, instruction->side_set_value);
    for (pin_num = sm->side_set_pins_base; pin_num < (sm->side_set_pins_base + sm->side_set_pins_num); pin_num++) {
        if (sm->side_set_pindirs) {
            hardware_set_gpio_dir(pin_num, value % 2);
            value = value >> 1;
        }
        else {
            hardware_set_gpio(pin_num, value %  2);
            value = value >> 1;
        }
    }
 }

/********************************
 **** Empty Instruction *********
 *******************************/

bool run_empty_instruction(instruction_t * instruction) {
    PRINTI("running empty instruction\n");
    bool completed = true;
    return completed;
}

bool run_empty_user_instruction(user_instruction_t * instruction) {
    PRINTI("running empty user instruction\n");
    bool completed = true;
    return completed;
}

/*****************************************
 **** Generic  Instruction Logic *********
 ****************************************/

bool exec_run_instruction(instruction_t * instruction) {
    bool completed;
    sm_t * sm = (sm_t *) instruction->executing_sm;
    if (!instruction->in_delay_state) {
        switch (instruction->instruction_type) {
            case jmp_instruction:    completed = run_jmp_instruction(instruction); break;
            case wait_instruction:   completed = run_wait_instruction(instruction); break;
            case nop_instruction:    completed = run_nop_instruction(instruction); break;
            case in_instruction:     completed = run_in_instruction(instruction); break;
            case out_instruction:    completed = run_out_instruction(instruction); break;
            case push_instruction:   completed = run_push_instruction(instruction); break;
            case pull_instruction:   completed = run_pull_instruction(instruction); break;
            case mov_instruction:    completed = run_mov_instruction(instruction); break;
            case set_instruction:    completed = run_set_instruction(instruction); break;
            case irq_instruction:    completed = run_irq_instruction(instruction); break;
            case empty_instruction:  completed = run_empty_instruction(instruction); break;
        };
        if (!sm->side_set_pins_optional && instruction->side_set_value < 0) {
            PRINT("Error: side set is not optional and no side set value set, assuming zero\n");
            instruction->side_set_value = 0;
        }
        if (instruction->side_set_value >= 0) run_side_set(instruction);
        if (completed && instruction->delay > 0) {
            instruction->in_delay_state = true;
            instruction->delay_left = instruction->delay-1;
            completed = false;
            PRINTD("not done, ... delaying\n");
        }
    }
    else {
        if (instruction->delay_left > 0) {
            PRINTD("...still delaying\n");
            instruction->delay_left--;
            completed = false;
        }
        else {
            PRINTD("...delay done\n");
            completed = true;
            instruction->in_delay_state = false;
        }
    }
    hardware_changed_gpio_history_update();
    if (completed) {
        instruction->not_completed = false;
        sm = (sm_t *) instruction->executing_sm;
        if (instruction->instruction_type == jmp_instruction) sm->pc = instruction->jmp_pc;
        else {
            if ( (instruction->instruction_type == out_instruction) && (instruction->destination == exec_destination) ) {
                PRINTI("pc set by instruction written\n");
            }
            else sm->pc++;
        }
    }
    else instruction->not_completed = true;
    return completed;
}

bool exec_run_user_instruction(user_instruction_t * instruction) {
    bool completed;
    instruction_or_user_instruction_t instr;
    user_processor_t * up;
    // process pre-delay first */
    if ((instruction->delay > 0) && !(instruction->in_delay_state)) {
        instruction->in_delay_state = true;
        instruction->delay_left = instruction->delay;
        completed = false;
    }
    else {
        instruction->delay_left--;
        if (instruction->in_delay_state && instruction->delay_left == 0) {
            PRINTI("delay completed\n");
            instruction->in_delay_state = false;
        }
        if (instruction->in_delay_state) {
            PRINTI("pre-delay before user instruction(%d left)\n", instruction->delay_left);
            completed = false;
        }
        else {
            switch (instruction->instruction_type) {
                case write_instruction:       completed = run_write_instruction(instruction); break;
                case read_instruction:        completed = run_read_instruction(instruction); break;
                case pin_instruction:         completed = run_pin_instruction(instruction); break;
                case empty_user_instruction:  completed = run_empty_user_instruction(instruction); break;
            };
        }
    }
    if (completed) {
        instruction->not_completed = false;
        up = (user_processor_t *) instruction->executing_up;
        up->pc++;
    }
    else instruction->not_completed = true;
    return completed;
}

