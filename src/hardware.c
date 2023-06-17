/*!
 * @file /hardware.c
 * @brief HARDWARE CONFIGURATION (container module)
 * @details
 * This defines the static configuration of pio hardware. The dynamic execution of instructions is defined separately.
 * This module does not provide any functionality, only the data itself, made availabe through structure definition and 
 * gets and sets (which are preferable to directly accessing structure elements, in order to be insulated against changes 
 * in the structure implementation).
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */
  
#include "hardware.h"
#include "ui.h"
#include "print.h"
#include <string.h>


/* hardware state data */
static pio_t pios[NUM_PIOS];
static sm_t  sms[NUM_PIOS * NUM_SMS];  /* sm index for the s'th sm in p'th pio is p*NUM_SMS + s */ 
static gpio_t gpios[NUM_GPIOS];
static user_processor_t user_processors[NUM_USER_PROCESSORS];
static irq_flags_t hardware_irq_flags[NUM_IRQ_FLAGS];

static int current_pio = -1;
static int current_sm = -1;
static int current_up = -1;

/* enumerators */

IMPLEMENT_ENUMERATOR(pio_t, hardware_pio, pios, NUM_PIOS)
IMPLEMENT_ENUMERATOR(sm_t, hardware_sm, sms, NUM_PIOS * NUM_SMS)
IMPLEMENT_ENUMERATOR(user_processor_t, hardware_user_processor, user_processors, NUM_USER_PROCESSORS)

/* some syntactic sugar macros */
#define CURRENT_PIO pios[current_pio]
#define CURRENT_SM sms[current_sm]
#define THIS_PIO pios[pio]
#define THIS_SM sms[sm]

/************************************************************************************************
  set 
 ************************************************************************************************/
    
void hardware_set_data(char * value) {
    snprintf(user_processors[current_up].data, STRING_MAX, "%s", value);
}


void hardware_set_pio(int pio, int line) {
    if (pio < 0 || pio > 1) PRINT("Error (line %d): pio must be 0 or 1\n", line);
    current_pio = pio;
}

void hardware_set_sm(int sm, int line) {
    if (sm < 0 || sm > 3) PRINT("Error (line %d): sm must be 0..3\n", line);
    current_sm = sm;
}

void hardware_set_up(uint8_t pnum, int line) {
    if (pnum >= NUM_USER_PROCESSORS) {
        PRINT("Error (line %d): max user processors is %d\n", line, NUM_USER_PROCESSORS-1);
        current_up = NUM_USER_PROCESSORS-1;
    }
    else current_up = pnum;
}

void hardware_set_set_pins(int base, int num_pins, int line) {
    CURRENT_SM.set_pins_base = base;
    CURRENT_SM.set_pins_num = num_pins;
}


void hardware_set_out_pins(int base, int num_pins, int line) {
    CURRENT_SM.out_pins_base = base;
    CURRENT_SM.out_pins_num = num_pins;
}


void hardware_set_in_pins(int base, int line) {
    CURRENT_SM.in_pins_base = base;
}

void hardware_set_side_set_pins(int base, int line) {
    CURRENT_SM.side_set_pins_base = base;
}
    
void hardware_set_side_set_count(int num_pins, int optional, int pindirs, int line) {
    CURRENT_SM.side_set_pins_num = num_pins;
    if (optional < 0 || optional > 1) {
        PRINT("Error: side set optional invalid value %d on line %d; assuming 1 (true, optional)\n", optional, line);
        optional = 1;
    }
    sms[current_sm].side_set_pins_optional = optional;
    if (pindirs < 0 || pindirs > 1) {
        PRINT("Error: side set pindirs invalid value %d on line %d; assuming 0 (false, not pindirs)\n", pindirs, line);
        pindirs = false;
    }
    CURRENT_SM.side_set_pindirs = pindirs;
}

void hardware_set_shiftctl_out(int dir, bool ap, int threshold, int line) {
    CURRENT_SM.shiftctl_out_shiftdir = dir;
    CURRENT_SM.autopull = ap;
    if (0<= threshold && threshold <= 31) CURRENT_SM.shiftctl_pull_thresh = threshold;
}

void hardware_set_shiftctl_in(int dir, bool ap, int threshold, int line) {
    CURRENT_SM.shiftctl_in_shiftdir = dir;
    CURRENT_SM.autopush = ap;
    if (0<= threshold && threshold <= 31) CURRENT_SM.shiftctl_push_thresh = threshold;
}

void hardware_set_status_sel(int sel, uint8_t level) {
    CURRENT_SM.fifo.EXECCTRL_STATUS_SEL = (sel != 0);
    CURRENT_SM.fifo.N = level;
}

void hardware_set_pio_instruction_cache(uint8_t pio) {
    int instruction;
    for (instruction=0; instruction < NUM_INSTRUCTIONS; instruction++) instruction_set_defaults(&(pios[pio].instructions[instruction]));
    THIS_PIO.next_instruction_location = 0;
}

void hardware_set_pio_defaults(uint8_t pio, uint8_t num) {
    THIS_PIO.irqs[0].value = false;
    THIS_PIO.irqs[1].value = false;
    THIS_PIO.this_num = num;
    hardware_set_pio_instruction_cache(pio);
}

void hardware_set_pios_defaults() {
    uint8_t pio;
    for (pio=0; pio < 2; pio++) hardware_set_pio_defaults(pio, pio);
}

void hardware_set_sm_defaults(uint8_t sm) {
    THIS_SM.pin_condition = -1; /* no pin condition set */
    THIS_SM.set_pins_base = 0;
    THIS_SM.set_pins_num = 0;
    THIS_SM.out_pins_base = 0;
    THIS_SM.out_pins_num = 0;
    THIS_SM.in_pins_base = 0;
    THIS_SM.side_set_count = 0;
    THIS_SM.side_set_pins_optional = true;
    THIS_SM.side_set_pindirs = false;
    THIS_SM.autopush = false;
    THIS_SM.autopull = false;
    THIS_SM.shiftctl_pull_thresh = -1; 
    THIS_SM.shiftctl_push_thresh = -1; 
    THIS_SM.shiftctl_out_shiftdir = false;
    THIS_SM.shiftctl_in_shiftdir = false;
    THIS_SM.program_name[0] = '\0';
    THIS_SM.pio_num = sm / 4;
    THIS_SM.pio = (void*) &(pios[sm / 4]);
    THIS_SM.this_num = sm % 4;
}

void hardware_reset_sm(uint8_t sm) {
    THIS_SM.pc = -1;
    THIS_SM.pc_temp = 0;
    THIS_SM.clock_tick = 0;
    fifo_init(&(THIS_SM.fifo), BIDI);
    THIS_SM.scratch_x = 0;
    THIS_SM.scratch_y = 0;
    THIS_SM.osr = 0;
    THIS_SM.isr = 0;
    THIS_SM.shift_in_count = 0;
    THIS_SM.shift_out_count = 0;
    THIS_SM.shift_in_resume_count = 0;
    THIS_SM.shift_out_resume_count = 0;
}

void hardware_reset_sms() {
    uint8_t sm;
    for (sm=0; sm < (NUM_PIOS * NUM_SMS); sm++) {
        hardware_reset_sm(sm);
    }
}

void hardware_set_sms_defaults() {
    uint8_t sm;
    for (sm=0; sm < (NUM_PIOS * NUM_SMS); sm++) {
        hardware_set_sm_defaults(sm);
        hardware_reset_sm(sm);
    }
}

void hardware_reset_user_processor_instruction_cache(int p) {
    int i;
    for (i=0; i<NUM_USER_INSTRUCTIONS; i++) {
        instruction_set_user_defaults(&(user_processors[p].instructions[i]));
    }
}

void hardware_reset_user_processor(int p) {
    user_processors[p].next_instruction_location = 0;
    user_processors[p].pc = -1;
    user_processors[p].this_num = p;
    user_processors[p].data[0] = '\0';
    hardware_reset_user_processor_instruction_cache(p);
}

void hardware_reset_user_processors() {
    int p;
    for (p=0; p<NUM_USER_PROCESSORS; p++) {
        hardware_reset_user_processor(p);
    }
}

void hardware_set_system_defaults() {
    int pio;
    current_pio = 0;
    current_sm = 0;
    hardware_set_pios_defaults();
    hardware_set_sms_defaults();
    hardware_reset_user_processors();
    instruction_set_global_default();
}

void hardware_reset() {
    hardware_reset_sms();
    hardware_reset_user_processors();
}


#define CHECK_GPIO(x) if (x < 0 || x >= NUM_GPIOS) {PRINT("Error: invalid gpio index"); return;}
#define CHECK_GPIO_B(x) if (x < 0 || x >= NUM_GPIOS) {PRINT("Error: invalid gpio index"); return false;}
#define CHECK_IRQ(x) if (x < 0 || x >= NUM_IRQS) {PRINT("Error: invalid irq index"); return;}
#define CHECK_IRQ_B(x) if (x < 0 || x >= NUM_IRQS) {PRINT("Error: invalid irq index"); return false;}

void hardware_set_gpio(uint8_t num, bool val) { CHECK_GPIO(num) gpios[num].value = val; } 
void hardware_set_gpio_dir(uint8_t num, bool dir) { CHECK_GPIO(num) gpios[num].pindir = dir; } 
bool hardware_get_gpio(uint8_t num) { CHECK_GPIO_B(num) return gpios[num].value; } 
bool hardware_get_gpio_dir(uint8_t num) { CHECK_GPIO_B(num) return gpios[num].pindir; } 

void hardware_set_irq(uint8_t irq_num, bool value) {pios[current_pio].irqs[irq_num].value = value;}

void hardware_set_program_name(char* name) {
   snprintf(CURRENT_SM.program_name, SYMBOL_MAX, "%s", name); 
   CURRENT_SM.pc = hardware_pio_set()->next_instruction_location;  /* the first executable statement after the .program directive is the first instruction to be executed by the current SM */
}


void hardware_init_current_sm_pc_if_needed(int8_t first_instruction_location) {
    sm_t *  current_sm = hardware_sm_set();
    if (current_sm->pc < 0) current_sm->pc = first_instruction_location;
}

void hardware_init_current_up_pc_if_needed(int8_t first_instruction_location) {
    user_processor_t *  current_up = hardware_user_processor_set();
    if (current_up->pc < 0) current_up->pc = first_instruction_location;
}

void hardware_set_pin_condition(int pin_num) {
    if (0 <= pin_num && pin_num <= 31) CURRENT_SM.pin_condition = pin_num;
}

bool hardware_irq_flag_set(uint8_t irq, bool set_or_clear) {
    if (irq < NUM_IRQ_FLAGS) {
        hardware_irq_flags[irq].set = set_or_clear;
        return true;
    }
    else return false;
}

void hardware_fifo_merge(fifo_mode_t mode) {
    fifo_t * f = &(CURRENT_SM.fifo);
    fifo_init(f, mode);
}


/************************************************************************************************
  get 
 ************************************************************************************************/

uint8_t hardware_pio_num_set() { return current_pio; }
uint8_t hardware_sm_num_set() { return current_sm; }

pio_t * hardware_pio_set() {return &(pios[current_pio]);}
sm_t *  hardware_sm_set()  {return &(sms[current_sm]);}

user_processor_t* hardware_user_processor_set() { return &(user_processors[current_up]); }

bool hardware_get_irq(uint8_t irq_num) {return pios[current_pio].irqs[irq_num].value;}

bool hardware_irq_flag_is_set(uint8_t irq) {
    if (irq < NUM_IRQ_FLAGS) {
        return hardware_irq_flags[irq].set;
    }
    else return false;
}

