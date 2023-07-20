/*!
 * @file /hardware.h
 * @brief HARDWARE CONFIGURATION (container module)
 * @details
 * This defines the static configuration of pio hardware. The dynamic execution of instructions is defined separately.
 * This module does not provide any functionality, only the data itself, made availabe through structure definition and 
 * gets and sets (which are preferable to directly accessing structure elements, in order to be insulated against changes 
 * in the structure implementation).
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "instruction.h"
#include "enumerator.h"
#include "fifo.h"

#define NUM_PIOS 2
// number of SMs *per PIO*
#define NUM_SMS                4
#define NUM_USER_PROCESSORS    2
#define NUM_GPIOS             32
#define NUM_IRQS               2
#define NUM_IRQ_FLAGS          8

#define STATUS_ALL_ONES  0xFFFFFFFF
#define STATUS_ALL_ZEROS 0

typedef enum {tx_level, rx_level, unconfigured_level} status_level_e;

typedef struct {
    bool value;
    bool pindir; /* true/1 is output */
} gpio_t;

typedef struct {
    bool value;
} irq_t;

typedef struct {
    /* SM 'hardware' */
    int32_t  pc;                        /* each SM has its own program instruction counter so they can execute instructions in parallel */
    fifo_t   fifo;  
    uint32_t scratch_x;                 /* x register */
    uint32_t scratch_y;                 /* y register */
    uint32_t osr;                       /* output shift register */
    uint32_t isr;                       /* input shift register */
    /* SM configuration */
    int32_t  first_pc;                  /* first_instruction to execute when reset */
    uint8_t  pin_condition;             /* gpio selected for pin condition (must be 0..31) */
    uint8_t  set_pins_base;             /* base gpio for set operations */
    uint8_t  set_pins_num;              /* number of gpios to set for set operations, starting at base */
    uint8_t  out_pins_base;             /* base gpio for output operations */
    uint8_t  out_pins_num;              /* number of gpios to set for output operations, starting at base */
    uint8_t  in_pins_base;              /* base gpio for input operations */
    uint8_t  side_set_pins_base;        /* base gpio for side set operations */
    uint8_t  side_set_pins_num;         /* number of gpios to set for side set operations, starting at base */
    bool     side_set_pins_optional;    /* whether side set is optional or required on each instruction */
    bool     side_set_pindirs;          /* whether side set affects the pin values or their direction */
    bool     autopush;                  /* whether autopush is enabled for this sm */
    bool     autopull;                  /* whether autopush is enabled for this sm */
    uint8_t  side_set_count;            /* the number of bits in an instruction reserved for side set */
    uint8_t  shift_in_count;            /* the number of shifts into the ISR  */
    uint8_t  shift_out_count;           /* the number of shifts out of the OSR */
    int      shiftctl_pull_thresh;      /* the number of bits in the OSR that can be shifted out before the OSR is considered empty */
    int      shiftctl_push_thresh;      /* the number of bits in the ISR that can be shifted in before the ISR is considered full */
    bool     shiftctl_out_shiftdir;     /* true shifts starting with LSB, false starts with MSB */
    bool     shiftctl_in_shiftdir;      /* true shifts starting with LSB, false starts with MSB */
    /* SM state data */
    uint32_t clock_tick;                /* each sm has their own clock; parallelelism is handled at a higher level by keeping keeping each SMs clock in sync */
    uint32_t pc_temp;                   /* for PC destination so that the instruction counter isn't updated until shifting is complete */
    uint8_t  shift_out_resume_count;    /* when an instruction is in a wait state, the number of output shifts is remembered for when it can resume */
    uint8_t  shift_in_resume_count;     /* when an instruction is in a wait state, the number of input shifts is remembered for when it can resume */
    uint16_t exec_machine_instruction;  /* when the destination is the PC, this holds the instruction that is being built up until it is complete */
    instruction_t exec_instruction;     /* when the destination is EXEC, this holds the decoded instruction that is to be executed */
    char     program_name[SYMBOL_MAX];  /* the name or the program currently loaded/running in this sm */
    void  *  pio;                       /* pointer up to the pio that this sm is part of */
    uint8_t  pio_num;
    uint8_t  this_num;
    bool     osr_empty;
    bool     isr_full;
} sm_t;

/* There are two pios, each with 2 irqs, 4 state machines, and instruction memory that is not currently modeled */
typedef struct {
    irq_t irqs[NUM_IRQS];
    instruction_t instructions[NUM_INSTRUCTIONS];
    int8_t  next_instruction_location; /* address of next place in pio program memory where an instruction can be added */
    uint8_t this_num;
} pio_t;

typedef struct {
    user_instruction_t instructions[NUM_USER_INSTRUCTIONS];
    int8_t  next_instruction_location; 
    int8_t  pc; /* user program counter */
    uint8_t this_num;
    char    data[STRING_MAX];
} user_processor_t;

typedef struct {
    bool    set; 
    bool    mapped_to_irq;
    uint8_t pio;
    uint8_t pio_num;
} irq_flags_t;


/************************************************************************************************************************
 *
 * HARDWARE CONFIGURATION FUNCTIONS
 *
 * Note: The real Pico system uses various means to configure the hardware, some at runtime through C function calls
 *       Since the regular C runtime on an ARM processor is not modeled, we need some way to simulate this mechanism.
 *       The approach used here is just to provide some pseudo instructions in the PIO program itself for setting these.
 *       This makes both the simulator implementation easier and makes writing test PIO programs easier, but it is not
 *       how the real pico system works so a TODO is to look at a more accurate way to do PIO configuration.
 *
 * Note: Since each PIO program is generally targeted at a single SM/PIO, there is also a pseudo instruction to set
 *       the SM and PIO for each program. Multiple programs can be define, each with their own SM and PIO. This means
 *       that if a given PIO program is to be run by multiple SM/PIOs then it would been to be duplicated (this is a 
 *       side effect of not having dynamic configuration through a C runtime system). 
 *
 * Note: Some configuration items are configured through the .define pseudo instruction, if they are global rather than
 *       configured per SM. (TODO: this all needs to be revisited to better match real pico configuration at some point.)
 *
 ************************************************************************************************************************/

/* the following set and get a "current context" (pio and sm) for programs being added and executed */
void hardware_set_pio(int pio, int line);
void hardware_set_sm(int sm, int line);
pio_t * hardware_pio_set();
sm_t *  hardware_sm_set();
uint8_t hardware_pio_num_set();
uint8_t hardware_sm_num_set();
uint8_t hardware_up_num_set();
void hardware_set_up(uint8_t pnum, int line);
user_processor_t* hardware_user_processor_set();

DEFINE_ENUMERATOR(pio_t, hardware_pio)
DEFINE_ENUMERATOR(sm_t, hardware_sm)
DEFINE_ENUMERATOR(user_processor_t, hardware_user_processor)

void hardware_set_pin_condition(int pin_num);
void hardware_set_set_pins(int base, int num_pins, int line);
void hardware_set_out_pins(int base, int num_pins, int line);
void hardware_set_in_pins(int base, int line);
void hardware_set_side_set_pins(int base, int line);
void hardware_set_side_set_count(int num_pins, int optional, int pindirs, int line);
void hardware_set_program_name(char* name);
void hardware_set_shiftctl_out(int dir, bool ap, int threshold, int line);
void hardware_set_shiftctl_in(int dir, bool ap, int threshold, int line);
void hardware_set_status_sel(int sel, uint8_t level);
void hardware_set_gpio(uint8_t num, bool val);
void hardware_set_gpio_dir(uint8_t num, bool val);
void hardware_set_irq(uint8_t irq_num, bool value);
void hardware_fifo_merge(fifo_mode_t mode);

void hardware_set_data(char * value);
              
void hardware_set_system_defaults();
void hardware_reset();

bool hardware_get_gpio(uint8_t num);
bool hardware_get_gpio_dir(uint8_t num);
bool hardware_get_irq(uint8_t irq_num);

void hardware_init_current_sm_pc_if_needed(int8_t first_instruction_location);
void hardware_init_current_up_pc_if_needed(int8_t first_instruction_location);

bool hardware_irq_flag_set(uint8_t irq, bool set_or_clear);
bool hardware_irq_flag_is_set(uint8_t irq);

/************************************************************************************************************************
 *
 * HARDWARE DEVICE Functions
 *
 * A "device" is a simulated peripheral (attached to GPIO pins)
 *
 * A simulated device has:
 * - an execution handler that is called to simulate an attached peripheral
 * - an display handler that is called to display the state of the simulated peripheral
 *
 * This hardware module is just a container for which devices are enabled and pointers to their handlers.
 * The execution module will make use of the execution handler and the UI module will use the display handler
 *
 * Simulated peripherals (devices) will call the register function to add itself to the list of devices.
 * The execution and UI modules will use the enumerator functions below to find out what devices are enabled and their handlers.
 *
 ************************************************************************************************************************/

#define MAX_DEVICES 5

typedef void (*device_execution_handler_t) ();
typedef int (*device_display_handler_t) ();

typedef struct {
    device_execution_handler_t   execution_handler;
    device_display_handler_t     display_handler;
    bool                         enabled;
    char                         name[SYMBOL_MAX];
} hardware_device_t;

void hardware_register_device(char * name, bool enabled, device_execution_handler_t exec, device_display_handler_t disp);

DEFINE_ENUMERATOR(hardware_device_t, hardware_device_enumerator);

#endif