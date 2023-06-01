/*!
 * @file /instruction.c
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

  
#include "instruction.h"
#include "print.h"
#include "hardware.h"
#include "ui.h"
#include "parser.h"
#include <string.h>
#include <assert.h>

#define CURRENT_INSTRUCTION hardware_pio_set()->instructions[hardware_pio_set()->next_instruction_location]
#define CURRENT_USER_INSTRUCTION hardware_user_processor_set()->instructions[hardware_user_processor_set()->next_instruction_location]

static int current_definition;
static int current_label;

static define_t definitions[NUM_DEFINES];
static label_location_t label_locations[NUM_INSTRUCTIONS]; /* There can't be more labels than instructions, or at least it isn't reasonable to have more labels than instructions. */

static user_variable_t symbol_table[NUM_VARS];

static bool prev_instruction_was_label;

uint8_t instruction_label_location(uint8_t line) { return label_locations[line].location; }
char *  instruction_label_symbol(uint8_t line){ return label_locations[line].label; }

/***********************************************************************************************************
 * helpers
 **********************************************************************************************************/

#define UNDEFINED(x) symbol_table[x].name[0] == 0
#define DEFINED(x)   symbol_table[x].name[0] != 0
#define UNDEFINE(x)  symbol_table[x].name[0] = 0;
#define NAME_MATCH(x,n) (strncmp(symbol_table[x].name, n, SYMBOL_MAX) == 0)
#define FORALLVARS(i) for (i=0; i<NUM_VARS; i++)

/***********************************************************************************************************
 * enumerators
 **********************************************************************************************************/

IMPLEMENT_ENUMERATOR(define_t, instruction_defines, definitions, NUM_DEFINES)

IMPLEMENT_ENUMERATOR(user_variable_t, user_variable, symbol_table, NUM_VARS)

/***********************************************************************************************************
 * set
 **********************************************************************************************************/

void instruction_vars_init() {
    int i;
    for (i=0; i<NUM_VARS; i++) {symbol_table[i].has_value = false; UNDEFINE(i) }
}

bool instruction_var_set(char * name, uint32_t val) {
    int i;
    FORALLVARS(i) { 
        if NAME_MATCH(i,name) {
            symbol_table[i].value = val;
            symbol_table[i].has_value = true;
            return true;
        }
    }
    return false;
}

bool instruction_var_define(char * name) {
    int i;
    FORALLVARS(i) { 
        if (UNDEFINED(i)) {
            snprintf(symbol_table[i].name, SYMBOL_MAX, "%s", name);
            return true;
        }
    }
    return false;
}

bool instruction_var_undefine(char * name) {
    int i;
    FORALLVARS(i) { 
        if NAME_MATCH(i,name) {
            UNDEFINE(i)
            return true;
        }
    }
    return false;
}

bool instruction_toggle_breakpoint(uint8_t line) {
  /* search through instructions in current pio for line, and toggle breakpoint if found, else return false because couldn't toggle breakpoint */
  pio_t * pio = hardware_pio_set();
  int instruction;
  for (instruction=0; instruction<NUM_INSTRUCTIONS; instruction++) {
    if (pio->instructions[instruction].line == line) {
      if (pio->instructions[instruction].is_breakpoint) pio->instructions[instruction].is_breakpoint = false;
      else pio->instructions[instruction].is_breakpoint = true;
      return true;
    }
  }
  return false;
}

void instruction_label_locations_init() {
    int i;
    for (i=0; i< NUM_INSTRUCTIONS; i++) {
        label_locations[i].label[0] = 0;
        label_locations[i].location = NO_LOCATION;
    }
}
              
void instruction_set_defaults(instruction_t *instr) {
    instr->instruction_type = empty_instruction;
    instr->side_set_value = -1;
    instr->in_delay_state = false;
    instr->delay = 0;
    instr->delay_left = 0;
    instr->condition = unset_condition;
    instr->polarity = 0;
    instr->wait_source = unset_wait_source;
    instr->source = unset_source;
    instr->destination = unset_destination;
    instr->if_full = 0;
    instr->if_empty = 0;
    instr->block = 0;
    instr->operation = unset_operation;
    instr->index_or_value = 0;
    instr->bit_count = 0;
    instr->location = NO_LOCATION;
    instr->address = -1;
    instr->is_breakpoint = false;
    instr->write_value = 0;
    instr->not_completed = false;
    instr->jmp_pc_set = false;
}

void instruction_set_user_defaults(user_instruction_t* instr) {
    instr->instruction_type = empty_user_instruction;
    instr->pin = -1;
    instr->delay = 0;
    instr->delay_left = -1;
    instr->in_delay_state = false;
    instr->not_completed = false;
    instr->var_name[0] = 0;
    instr->continue_user = false;
}

void instruction_set_definition_defaults() {
    current_definition = 0;
    for (int i=0; i<NUM_DEFINES; i++) definitions[i].defined = false;
}

void instruction_set_global_default() {
    instruction_set_definition_defaults();
    current_label = 0;
    instruction_label_locations_init();
    instruction_vars_init();
}

bool instruction_add(instruction_t* instr) {
    if (hardware_pio_set()->next_instruction_location == NUM_INSTRUCTIONS) {
        PRINT("\nERROR line %d: number of SM instructions (%d) exceeded\n", instr->line, NUM_INSTRUCTIONS);
        return false;
    }
    PRINTD("\n---->adding instruction Line: %d\n", instr->line);
    CURRENT_INSTRUCTION.line = instr->line;
    CURRENT_INSTRUCTION.instruction_type = instr->instruction_type;
    CURRENT_INSTRUCTION.side_set_value = instr->side_set_value;
    CURRENT_INSTRUCTION.delay = instr->delay;
    CURRENT_INSTRUCTION.condition = instr->condition;
    CURRENT_INSTRUCTION.polarity = instr->polarity;
    CURRENT_INSTRUCTION.wait_source = instr->wait_source;
    CURRENT_INSTRUCTION.source = instr->source;
    CURRENT_INSTRUCTION.destination = instr->destination;
    CURRENT_INSTRUCTION.if_full = instr->if_full;
    CURRENT_INSTRUCTION.if_empty = instr->if_empty;
    CURRENT_INSTRUCTION.block = instr->block;
    CURRENT_INSTRUCTION.operation = instr->operation;
    CURRENT_INSTRUCTION.index_or_value = instr->index_or_value;
    CURRENT_INSTRUCTION.bit_count = instr->bit_count;
    CURRENT_INSTRUCTION.write_value = instr->write_value;
    CURRENT_INSTRUCTION.location = instr->location;
    snprintf(CURRENT_INSTRUCTION.label, SYMBOL_MAX, "%s", instr->label);
    CURRENT_INSTRUCTION.executing_sm_num = hardware_sm_num_set();
    CURRENT_INSTRUCTION.pio = hardware_pio_set();
    CURRENT_USER_INSTRUCTION.executing_sm = (void *) hardware_sm_set();
    CURRENT_INSTRUCTION.pio =(void *)  hardware_pio_set();
    hardware_init_current_sm_pc_if_needed(hardware_pio_set()->next_instruction_location);  /* first instruction added for this sm will be the first to execute on this sm */
    CURRENT_INSTRUCTION.address = hardware_pio_set()->next_instruction_location;
    hardware_pio_set()->next_instruction_location++;
    prev_instruction_was_label = false;
    return true;
}

bool instruction_user_add(user_instruction_t* instr){
    if (hardware_user_processor_set()->next_instruction_location == NUM_USER_INSTRUCTIONS) {
        PRINT("\nERROR line %d: number of user instructions (%d) exceeded\n", instr->line, NUM_USER_INSTRUCTIONS);
        return false;
    }
    PRINTD("\n---->adding user instruction Line: %d, value:%0x\n", instr->line, instr->value);
    //CURRENT_USER_INSTRUCTION.pio = hardware_pio_set();
    CURRENT_USER_INSTRUCTION.line = instr->line;
    CURRENT_USER_INSTRUCTION.instruction_type = instr->instruction_type;
    CURRENT_USER_INSTRUCTION.value = instr->value;
    CURRENT_USER_INSTRUCTION.pin = instr->pin;
    CURRENT_USER_INSTRUCTION.set_high = instr->set_high;
    CURRENT_USER_INSTRUCTION.delay = instr->delay;
    CURRENT_USER_INSTRUCTION.continue_user = instr->continue_user;
    CURRENT_USER_INSTRUCTION.delay_left = -1;
    CURRENT_USER_INSTRUCTION.in_delay_state = false;
    CURRENT_USER_INSTRUCTION.executing_sm = (void *) hardware_sm_set();
    snprintf(CURRENT_USER_INSTRUCTION.var_name, SYMBOL_MAX, "%s", instr->var_name);
    hardware_init_current_up_pc_if_needed(hardware_user_processor_set()->next_instruction_location);  /* first instruction added for this sm will be the first to execute on this sm */
    CURRENT_INSTRUCTION.address = hardware_user_processor_set()->next_instruction_location;
    hardware_user_processor_set()->next_instruction_location++;
    return true;
}                                     

void instruction_add_define(char* s, int v, int line) {
    snprintf(definitions[current_definition].symbol, SYMBOL_MAX, "%s", s);
    definitions[current_definition].value = v;
    definitions[current_definition].defined = true;
    current_definition++;
}

void instruction_add_label(char* l) {
      pio_t * current_pio = hardware_pio_set();
      int len = strnlen(l,LABEL_MAX);
      PRINTD("---->adding label: %s\n", l);
      snprintf(label_locations[current_label].label, LABEL_MAX, "%s", l);
      PRINTD("---->added label: %s\n", label_locations[current_label].label);
      label_locations[current_label++].location = current_pio->next_instruction_location;
      prev_instruction_was_label = true;
}

/* return positive number if error */
int instruction_fix_forward_labels() {
   instruction_t * instr;
   int i, fixed;
   /* go through all instructions in all pios, looking at jmp instructions */
   fixed = 0;
   FOR_ENUMERATION(pio, pio_t, hardware_pio) {
     for (i=0; i < pio->next_instruction_location; i++) {
       instr = &(pio->instructions[i]);
       if (instr->instruction_type == jmp_instruction && instr->location == NO_LOCATION) {
         instr->location = instruction_find_label(instr->label);
         if (instr->location == NO_LOCATION) {
             status_msg("unable to fix reference to %s\n", instr->label);
             return instr->line;
         }
         else fixed++;
       }
     }
   }
   status_msg("fixed %d forward references\n", fixed);
   return 0;
}

/***********************************************************************************************************
 * get
 **********************************************************************************************************/

int instruction_num_defines() {return current_definition;}
int instruction_num_labels() {return current_label;}

bool instruction_var_get(char * name, uint32_t * value) {
    int i;
    FORALLVARS(i) { 
        if NAME_MATCH(i,name) {
            if (symbol_table[i].has_value) {
               *value = symbol_table[i].value;
               return true;
            }
            else return false;
        }
    }
    return false;
}


int instruction_find_definition(char *s, int value_if_not_found) {
    int i;
    for (i=0; i<current_definition; i++) {
        if (strncmp(s,definitions[i].symbol, SYMBOL_MAX) == 0) return definitions[i].value;
    }
    return value_if_not_found;
}

uint8_t instruction_find_label(char * l) {
    uint8_t i;
    for (i=0; i<current_label; i++) {
      if (strncmp(l,label_locations[i].label, SYMBOL_MAX) == 0) return i;
    }
    return NO_LOCATION;
} 

void print_instruction(instruction_t* instr);

/************************************************************************************************************************
 * internals
 ***********************************************************************************************************************/

void instruction_for_line(uint8_t line, instruction_or_user_instruction_t * instr) {
  /* search through instructions in pios and user processors for line, and return pointer to instruction found */
  uint8_t instruction;
  FOR_ENUMERATION(pio, pio_t, hardware_pio) {
      for (instruction=0; instruction<NUM_INSTRUCTIONS; instruction++) {
        if (pio->instructions[instruction].line == line) {
            instr->instruction_type = _instruction;
            instr->ioru.instruction_ptr = &(pio->instructions[instruction]);
            return;
        }
      }
  }
  FOR_ENUMERATION(up, user_processor_t, hardware_user_processor) {
      for (instruction=0; instruction<NUM_USER_INSTRUCTIONS; instruction++) {
        if (up->instructions[instruction].line == line) {
            instr->instruction_type = _user_instruction;
            instr->ioru.user_instruction_ptr = &(up->instructions[instruction]);
            return;
        }
      }
  }
  instr->instruction_type = _no_instruction;
  instr->ioru.instruction_ptr = NULL;
}

bool instruction_is_breakpoint(uint8_t line) {
  /* search through instructions in pios and user processors for line, and return status of breakpoint; default to false if not found */
  int instruction;
  FOR_ENUMERATION(pio, pio_t, hardware_pio) {
      for (instruction=0; instruction<NUM_INSTRUCTIONS; instruction++) {
        if (pio->instructions[instruction].line == line) return pio->instructions[instruction].is_breakpoint;
      }
  }
  FOR_ENUMERATION(up, user_processor_t, hardware_user_processor) {
      for (instruction=0; instruction<NUM_USER_INSTRUCTIONS; instruction++) {
        if (up->instructions[instruction].line == line) return up->instructions[instruction].is_breakpoint;
      }
  }
  return false;
}

int instruction_next_line() {
    pio_t * pio = hardware_pio_set();
    sm_t * sm = hardware_sm_set();
    /* return the line number of the instruction pointed to by the current sm's pc */
    return pio->instructions[sm->pc].line;
}

int instruction_current_delay_remaining() {
    pio_t * pio = hardware_pio_set();
    sm_t * sm = hardware_sm_set();
    return pio->instructions[sm->pc].delay_left;
}

