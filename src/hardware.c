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
static ih_processor_t ih_processors[NUM_IH_PROCESSORS];
static hardware_irq_flag_t hardware_irq_flags[NUM_IRQ_FLAGS];

static int current_pio = -1;
static int current_sm = -1;
static int current_up = -1;
static int current_ih = -1;

static char current_program_name[SYMBOL_MAX];

static user_instruction_context_e user_instruction_context;
user_instruction_context_e hardware_get_user_instruction_context()  {return user_instruction_context;}


/* enumerators */

IMPLEMENT_ENUMERATOR(pio_t, hardware_pio, pios, NUM_PIOS)
IMPLEMENT_ENUMERATOR(sm_t, hardware_sm, sms, NUM_PIOS * NUM_SMS)
IMPLEMENT_ENUMERATOR(user_processor_t, hardware_user_processor, user_processors, NUM_USER_PROCESSORS)
IMPLEMENT_ENUMERATOR(ih_processor_t, hardware_ih_processor, ih_processors, NUM_IH_PROCESSORS)
IMPLEMENT_ENUMERATOR(hardware_irq_flag_t, hardware_irq_flag, hardware_irq_flags, NUM_IRQ_FLAGS)

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


int hardware_set_pio(int pio, int line) {
    if (pio < 0 || pio > 1) {
      PRINT("Error (line %d): pio must be 0 or 1\n", line+1);
      return -1;
    }
    current_pio = pio;
    return 0;
}

int hardware_set_sm(int sm, int line) {
    if (sm < 0 || sm > 3) {
      PRINT("Error (line %d): sm must be 0..3\n", line+1);
      return -1;
    }
    if (current_pio < 0) current_sm = sm;
    else current_sm = current_pio * 4 + sm;
    if (current_program_name) snprintf(CURRENT_SM.program_name, SYMBOL_MAX, "%s", current_program_name);
    return 0;
}

void hardware_set_up(uint8_t pnum, int line) {
    if (pnum >= NUM_USER_PROCESSORS) {
        PRINT("Error (line %d): max user processors is %d\n", line, NUM_USER_PROCESSORS-1);
        current_up = NUM_USER_PROCESSORS-1;
    }
    else current_up = pnum;
    user_instruction_context = up_context;
}

void hardware_set_ih(uint8_t pnum, int line) {
    if (pnum >= NUM_IH_PROCESSORS) {
        PRINT("Error (line %d): max ih processors is %d\n", line, NUM_IH_PROCESSORS-1);
        current_ih = NUM_IH_PROCESSORS-1;
    }
    else current_ih = pnum;
    user_instruction_context = ih_context;
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
    THIS_PIO.irqs[0].enabled = false;
    THIS_PIO.irqs[1].enabled = false;
    THIS_PIO.this_num = num;
    hardware_set_pio_instruction_cache(pio);
}

void hardware_set_pios_defaults() {
    uint8_t pio;
    for (pio=0; pio < 2; pio++) hardware_set_pio_defaults(pio, pio);
}

void hardware_set_sm_defaults(uint8_t sm) {
    THIS_SM.pc = -1;
    THIS_SM.wrap = -1;
    THIS_SM.wrap_target = -1;
    THIS_SM.first_pc = -1;
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
    THIS_SM.pc = THIS_SM.first_pc;
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
    THIS_SM.osr_empty = true;
    THIS_SM.isr_full = false;
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

void hardware_reset_ih_processor_instruction_cache(int p) {
    int i;
    for (i=0; i<NUM_USER_INSTRUCTIONS; i++) {
        instruction_set_user_defaults(&(ih_processors[p].instructions[i]));
    }
}

void hardware_reset_ih_processor(int p) {
    ih_processors[p].next_instruction_location = 0;
    ih_processors[p].pc = -1;
    ih_processors[p].this_num = p;
    ih_processors[p].data[0] = '\0';
    hardware_reset_ih_processor_instruction_cache(p);
}

void hardware_reset_ih_processors() {
    int p;
    for (p=0; p<NUM_IH_PROCESSORS; p++) {
        hardware_reset_ih_processor(p);
    }
}

void hardware_reset_irq_flags() {
 int i;
  for (i=0; i < NUM_IRQ_FLAGS; i++) {
    hardware_irq_flags->set = false; 
    hardware_irq_flags->mapped_to_irq = false;
  }
}

void hardware_set_system_defaults() {
    int pio;
    current_pio = 0;
    current_sm = 0;
    hardware_set_pios_defaults();
    hardware_set_sms_defaults();
    hardware_reset_user_processors();
    hardware_reset_ih_processors();
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

void hardware_set_irq(uint8_t irq_num, bool value) {pios[current_pio].irqs[irq_num].set = value;}

void hardware_set_program_name(char* name) {
   snprintf(current_program_name, SYMBOL_MAX, "%s", name); 
}


void hardware_init_current_sm_pc_if_needed(int8_t first_instruction_location) {
    sm_t *  current_sm = hardware_sm_set();
    PRINTD("checking first instruction for %d\n", current_sm->this_num);
    if (current_sm->first_pc < 0) {
        current_sm->first_pc = current_sm->pc = first_instruction_location;
        PRINTD("sm %d first instruction = %d\n", current_sm->this_num, first_instruction_location);
    }
    else {PRINTD("sm %d PC already set to %d\n", current_sm->this_num, current_sm->pc)};
}

void hardware_init_current_up_pc_if_needed(int8_t first_instruction_location) {
    user_processor_t *  current_up = hardware_user_processor_set();
    if (current_up->pc < 0) current_up->pc = first_instruction_location;
}

void hardware_set_wrap(int line) {
    PRINT("line: %d - setting wrap for pio %d to %d\n", line, current_pio, pios[current_pio].next_instruction_location - 1);
    sms[current_sm].wrap = pios[current_pio].next_instruction_location - 1;
}

void hardware_set_wrap_target(int line) {
    PRINT("line: %d - setting wrap_target for pio %d to %d\n", line, current_pio, pios[current_pio].next_instruction_location);
    sms[current_sm].wrap_target = pios[current_pio].next_instruction_location;
}

void hardware_set_pin_condition(int pin_num) {
    if (0 <= pin_num && pin_num <= 31) CURRENT_SM.pin_condition = pin_num;
}

bool hardware_irq_flag_set(uint8_t irq, bool set_or_clear) {
    if (irq < NUM_IRQ_FLAGS) {
        hardware_irq_flags[irq].set = set_or_clear;
        return true;
    }
    else {
        PRINT("error: invalid irq flag %d\n", irq);
        return false;
    }
}

void hardware_fifo_merge(fifo_mode_t mode) {
    fifo_t * f = &(CURRENT_SM.fifo);
    fifo_init(f, mode);
}

void hardware_enable_irq_handler(uint8_t pio, uint8_t irq, uint8_t flag, uint8_t line) {
    if (pio >= NUM_PIOS) {
        PRINT("Error line %d: can't set pio %d irq handler; max num PIOs is %d\n", line, pio, NUM_PIOS);
        return;
    }
    if (irq >= NUM_IRQS) {
        PRINT("Error line %d: can't set irq %d as irq handler; max num IRQss is %d\n", line, pio, NUM_IRQS);
        return;
    }
    if (irq >= 4) {
        PRINT("Error line %d: can't flag irq %d as irq handler; only flag for IRQs are 0..3\n", line, flag);
        return;
    }
    pios[pio].irqs[irq].enabled = true;
    pios[pio].irqs[irq].flag = flag;
    hardware_irq_flags[flag].mapped_to_irq = true;
    hardware_irq_flags[flag].pio = pio;
    hardware_irq_flags[flag].ih = &(ih_processors[current_ih]);
}

/************************************************************************************************
  get 
 ************************************************************************************************/

uint8_t hardware_pio_num_set() { return current_pio; }
uint8_t hardware_sm_num_set() { return current_sm; }
uint8_t hardware_up_num_set() { return current_up; }
uint8_t hardware_ih_num_set() { return current_ih; }


pio_t * hardware_pio_set() {return &(pios[current_pio]);}
sm_t *  hardware_sm_set()  {return &(sms[current_sm]);}

user_processor_t* hardware_user_processor_set() { return &(user_processors[current_up]); }

ih_processor_t* hardware_ih_processor_set() { return &(ih_processors[current_ih]); }

bool hardware_get_irq(uint8_t irq_num) {return pios[current_pio].irqs[irq_num].set;}

bool hardware_irq_flag_is_set(uint8_t irq) {
    if (irq < NUM_IRQ_FLAGS) {
        return hardware_irq_flags[irq].set;
    }
    else return false;
}

/************************************************************************************************
  devices simulated 
 ************************************************************************************************/

hardware_device_t hardware_devices[MAX_DEVICES];
static int last_device = -1;

void hardware_register_device(char * name, bool enabled, device_execution_handler_t exec, device_display_handler_t disp) {
    if (++last_device == MAX_DEVICES) return;
    strncpy(hardware_devices[last_device].name, name, SYMBOL_MAX);
    hardware_devices[last_device].enabled = enabled;
    hardware_devices[last_device].execution_handler = exec;
    hardware_devices[last_device].display_handler = disp;
    PRINTI("device %s registered\n", name);
}

IMPLEMENT_ENUMERATOR(hardware_device_t, hardware_device_enumerator, hardware_devices, MAX_DEVICES)
    