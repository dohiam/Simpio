/*!
 * @file /instruction.h
 * @brief PIO & User Instructions (container module)
 * @details
 * This module provides all intruction information for PIO and user instructions,
 * including types, structs, and get/set functions. It also provides functionality
 * for printing instructions.
 * Note: instruction memory is not modeled; instead of 32 uinit32's that are 
 * encoded/decoded, instructions are a structure that is already 'decoded'
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */


#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "stdint.h"
#include "stdbool.h"
#include "enumerator.h"

#define NUM_INSTRUCTIONS 132
#define NUM_USER_INSTRUCTIONS 32
#define NUM_DEFINES 32
#define SYMBOL_MAX 32
#define LABEL_MAX SYMBOL_MAX
#define NO_LOCATION 255

typedef enum { jmp_instruction, wait_instruction, nop_instruction, in_instruction, out_instruction, push_instruction, pull_instruction, mov_instruction, set_instruction, irq_instruction, empty_instruction }instruction_e;
typedef enum { read_instruction, write_instruction, pin_instruction, data_instruction, repeat_instruction, exit_instruction, empty_user_instruction } user_instruction_e;
typedef enum { always, x_zero, y_zero, x_decrement, y_decrement, x_not_equal_y, pin_condition, not_osre, unset_condition } condition_e;
typedef enum { gpio_source, pin_source, irq_source , reserved_wait_source, unset_wait_source } wait_source_e;
typedef enum { pins_source, x_source, y_source, null_source, isr_source, osr_source, status_source, reserved_source, unset_source } source_e;
typedef enum { pins_destination, x_destination, y_destination, reserved_destination, exec_destination, pc_destination, isr_destination, osr_destination, 
               pindirs_destination, null_destination, unset_destination } destination_e;
typedef enum { no_operation, invert, bit_reverse, clear_operation, set_operation, wait_operation, nowait_operation,reserved_operation, unset_operation } operation_e;
typedef enum { data_write, data_read, data_readln, data_print, data_clear, data_set, no_data_operation } data_operation_e;

typedef struct {
    char    symbol[SYMBOL_MAX];
    uint8_t value; 
    bool    defined;
} define_t;

typedef struct {
    char label[LABEL_MAX];   /* TODO: revisit 32 character label limit */
    uint8_t location; /* 0 .. 31 instruction where this label points to */ 
} label_location_t;

typedef struct {
    instruction_e     instruction_type; 
    /* fields common to all instruction types */
    int8_t            side_set_value;
    uint8_t           delay;
    void *            executing_sm;      /* the sm executing this instruction; can't declare sm_t because that would lead to circular header file inclusion */
    void *            pio;
    uint8_t           executing_sm_num;
    /* the following are various fields in different instruction types; some could be put into a sub-union because not all intructions use all the following fields */
    condition_e       condition;
    bool              polarity;
    wait_source_e     wait_source;
    source_e          source;
    destination_e     destination;
    bool              if_full;
    bool              if_empty;
    bool              block;
    operation_e       operation;
    uint32_t           index_or_value;     /* meaning depends on the instruction; it may be a value or a GPIO number or IRQ number or ... */
    uint8_t           bit_count;
    uint8_t           write_value;        /* for meta instruction write */
    bool              clear;
    bool              wait;
    bool              is_relative;
    int8_t            address;            /* the address of this instruction */
    /* housekeeping data */
    bool              in_delay_state;     /* when the instruction has a delay value, this indicates whther it is waiting for the delay count to be reached */
    uint8_t           delay_left;         /* when in a delay state, this holds how much time is left to delay */
    uint8_t           jmp_pc;             /* holds the jump location, in case need to delay before actually jumping */
    uint8_t           location;           /* index into label_locations where the location of the jump label can be fount */
    uint8_t           line;               /* line number of this instruction in the source file */
    bool              is_breakpoint;      /* true if a breakpoint has been set on this instruction */
    char              label[LABEL_MAX];   /* needed if can't resolve label on the first pass */
    bool              not_completed;
    bool              jmp_pc_set;
} instruction_t;

typedef struct {
    user_instruction_e instruction_type;
    data_operation_e   data_operation_type;
    void *             executing_up;      /* the up executing this instruction; can't declare type because that would lead to circular header file inclusion */
    void *             executing_sm;      /* to determine which FIFO this user processor instruction should use read & write  */
    char *             data_ptr;
    int                data_index;
    bool               data_indexing;
    int                max_read_index;
    uint32_t           value;
    int8_t             pin;
    bool               set_high;
    uint8_t            delay;
    int                delay_left;
    bool               in_delay_state;
    bool               delay_completed;
    bool               not_completed;
    uint8_t            line;
    bool               is_breakpoint;      /* true if a breakpoint has been set on this instruction */
    int                address;            /* the address of this instruction */
    char               var_name[SYMBOL_MAX];
    bool               continue_user;
} user_instruction_t;

typedef struct {
    char     name[SYMBOL_MAX];
    uint32_t value;
    bool     has_value;
} user_variable_t;

#define NUM_VARS 10

void instruction_vars_init();
bool instruction_var_set(char * name, uint32_t val);
bool instruction_var_get(char * name, uint32_t * value);
bool instruction_var_define(char * name);
bool instruction_var_undefine(char * name);

DEFINE_ENUMERATOR(user_variable_t, user_variable)

/************************************************************************************************************************
 *
 * TODO: revisit multiple SM/program execution support
 *
 ************************************************************************************************************************/

/* TODO: check usage and delete */
typedef struct {
    char     program_name[SYMBOL_MAX];
    uint8_t  side_set_bit_count;
    bool     side_set_value_optional;
    bool     side_sets_pindirs;
} program_t;


/************************************************************************************************************************
 *
 *
 ************************************************************************************************************************/

void instruction_set_defaults(instruction_t *instr);
void instruction_set_user_defaults(user_instruction_t* instr);

// reset just the state data (so it can be executed again)
void instruction_reset(instruction_t *instr);
void instruction_user_reset(user_instruction_t* instr);

void instruction_set_global_default();

/* the following adds an instruction for the current program's target sm and pio */    
bool instruction_add(instruction_t* instr);

bool instruction_user_add(user_instruction_t* instr);
void instruction_add_data(user_instruction_t* instr, char * data);

void instruction_add_define(char* s, int v, int line);

int instruction_find_definition(char *s, int value_if_not_found);

int instruction_num_defines();
int instruction_num_labels();

/* note: when adding a label, the target is the next instruction (in the current program's context) */
void instruction_add_label(char * l);

uint8_t instruction_find_label(char * l);

uint8_t instruction_label_location(uint8_t line);
char *  instruction_label_symbol(uint8_t line);

/* the following accounts for labels that were not found in the first pass because they were defined later in the program */
int instruction_fix_forward_labels();
/* zero means all labels are resolved, positive number is the line of the first instruction whose label could not be resolved */

typedef union {
    instruction_t * instruction_ptr;
    user_instruction_t * user_instruction_ptr;
} instruction_or_user_instruction_u;

typedef enum {_instruction, _user_instruction, _no_instruction} instruction_or_user_instruction_e;

typedef struct {
    instruction_or_user_instruction_e instruction_type;
    instruction_or_user_instruction_u ioru;
} instruction_or_user_instruction_t;

void instruction_for_line(uint8_t line, instruction_or_user_instruction_t * instr); 
bool instruction_is_breakpoint(uint8_t line);
bool instruction_toggle_breakpoint(uint8_t line);

int instruction_next_line();
int instruction_current_delay_remaining();

DEFINE_ENUMERATOR(define_t, instruction_defines)


#endif