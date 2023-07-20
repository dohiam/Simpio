/*!
 * @file /print.c
 * @brief Simpio print utilties
 * @details
 * Includes both printf and status_msg output, but note that the same level of information is not 
 * necessarily provided each way.
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "print.h"
#include "hardware.h"
#include "ui.h"

bool print_ui = true;
int  print_level = 0;

void set_print_ui(bool ui) { print_ui = ui; }
void set_print_level(int level) { print_level = level; }

#define MAX_NUMBER_OF_LINES 1000
#define ESTIMATED_LINE_SIZE 80
#define TEXT_BUFFER_SIZE MAX_NUMBER_OF_LINES * ESTIMATED_LINE_SIZE

static char print_line_buffer[TEXT_BUFFER_SIZE];
static int  print_line_starts[MAX_NUMBER_OF_LINES];
static int  print_line_ends[MAX_NUMBER_OF_LINES];

/**********************************************************************************
 * printfs 
 **********************************************************************************/

#define LSB_MASK 1
#define MSB_MASK_32 0x80000000
#define MSB_MASK_16 0x8000

//print the number of zeros in between ones in a binary number
void printf_zero_pattern(uint32_t n, bool direction) {
    int zero_count = 0;
    int i;
    uint32_t masked;
    if (direction) {
        printf("right to left number of zeros before a one: ");
        for (i=0; i<32; i++) {
            masked = n &LSB_MASK;
            n >>= 1;
            if (!masked) zero_count++;
            else {
                printf("%d ", zero_count);
                zero_count = 0;
            }
        }
        printf("%d ", zero_count);
    }
    else {
        printf("left to right number of zeros before a one: ");
        for (i=0; i<32; i++) {
            masked = n & MSB_MASK_32;
            n <<= 1;
            if (!masked) zero_count++;
            else {
                printf("%d ", zero_count);
                zero_count = 0;
            }
        }
        printf("%d ", zero_count);
    }
}

static void printf_if_full(bool iff) {
    if (iff) {printf("if_full true "); ;}
    else {printf("if_full false ");}
}
                                     
static void printf_if_empty(bool iff) {
    if (iff) {printf("if_empty true "); ;}
    else {printf("if_empty false ");}
}

static void printf_block(bool bl) {
    if (bl) {printf("instruction will block/wait ");}
    else {printf("instruction will not block/wait ");}
}
                                     
static void printf_operation(operation_e op) {
    switch (op) {
        case no_operation: printf("operation: none "); break;
        case invert: printf("operation: invert "); break;
        case bit_reverse: printf("operation: bit reverse "); break;
        case clear_operation: printf("operation: clear "); break;
        case wait_operation: printf("operatino: wait "); break;
        case unset_operation: printf("operation is not set !"); break;
    };
}
                                     
static void printf_index(uint8_t iov) {
    printf("index: %2d ", iov);
}
                                     
static void printf_value(uint32_t iov) {
    printf("value: %08x ", iov);
}
                                     
static void printf_bit_count(uint8_t sc) {
    printf("shift count: %2d ", sc);
}
                                     
static void printf_clear(bool cl) {
    if(cl) {printf("clear: true ");}
    else {printf("clear: false ");}
}
                                     
static void printf_wait(bool w) {
    if(w) {printf("wait: true ");}
    else {printf("wait: false ");}
}
                                     
static void printf_location(instruction_t * instr) {
    pio_t * pio = (pio_t *) instr->pio;
    printf("label: %s ", instr->label);
    printf("location: %d ", instr->location);
    printf("to-line %d ", pio->instructions[instr->location].line);           
}

static void printf_address(int addr) {   
   printf("address: %2d ", addr);
}

static void printf_side_set_value(int8_t side_set_value) {
    printf("side set value: %2d  ", side_set_value);   
}
                                     
static void printf_delay(uint8_t delay_value) {
    printf("delay value: %2d  ", delay_value);   
}
                                     
static void printf_jmp_condition(condition_e condition) {
    switch (condition) {
        case unset_condition: 
        case always: printf("always "); break;
        case x_zero: printf("if x is zero "); break;
        case y_zero: printf("if y is zero "); break;
        case x_decrement: printf("if x is not zero, decrement and jump "); break;
        case y_decrement: printf("if y is not zero, decrement and jump "); break;
        case x_not_equal_y: printf("if x != y    "); break;
        case pin_condition: printf("if pin indicated by EXECCTRL_JMP_PIN is high "); break;
        case not_osre: printf("if OSRE is not empty "); break;
    };
}
                                     
static void printf_polarity(bool p) {
    if (p) {printf("polarity: one ");}
    else {printf("polarity: zero ");}
}
                                     
static void printf_wait_source(wait_source_e w) {
    switch(w) {
        case gpio_source: printf("wait source: gpio "); break;
        case pin_source: printf("wait_source: pin "); break;
        case irq_source: printf("wait_source: irq "); break;
        case unset_wait_source: printf("wait source not set ! "); break;
    };
}
                                     
static void printf_source(source_e s) {
    switch (s) {
        case pins_source: printf("source: pins selected by PINCTRL_IN_BASE + bit count "); break;
        case x_source: printf("source: x scratch register "); break;
        case y_source: printf("source: y scratch register "); break;
        case null_source: printf("source: null "); break;
        case isr_source: printf("source: isr "); break;
        case osr_source: printf("source: osr "); break;
        case status_source: printf("source: status as specified by EXECCTRL_STATUS_SEL "); break;
        case unset_source: printf("source is not set ! "); break;
    };
}
                                     
static void printf_destination(destination_e d) {
    switch (d) {
        case pins_destination: printf("destination: pins selected by PINCTRL_IN_BASE + bit count "); break;
        case x_destination: printf("destination: x scratch register "); break;
        case y_destination: printf("destination: y scratch register "); break;
        case null_destination: printf("destination: null "); break;
        case pindirs_destination: printf("destination: pindirs "); break;
        case pc_destination: printf("destination: instruction counter "); break;
        case isr_destination: printf("destination: isr "); break;
        case exec_destination: printf("destinatino: EXEC "); break;
        case unset_destination: printf("destination is not set ! "); break;
    };
}
static void printf_instruction(instruction_t* instr) {
    printf("Line: %d ", instr->line);
    switch (instr->instruction_type) {
        case jmp_instruction: 
            printf("instruction: JMP "); 
            printf_jmp_condition(instr->condition);
            printf_location(instr);
            break;
        case wait_instruction: 
            printf("instruction: WAIT "); 
            printf_polarity(instr->polarity);
            printf_wait_source(instr->wait_source);
            printf_index(instr->index_or_value);
            break;
        case nop_instruction: 
            printf("instruction: NOP "); 
            break; 
        case in_instruction: 
            printf("instruction: IN "); 
            printf_source(instr->source);
            printf_bit_count(instr->bit_count);
            break;
        case out_instruction: 
            printf("instruction: OUT "); 
            printf_destination(instr->destination);
            printf_bit_count(instr->bit_count);
            break;
        case push_instruction: 
            printf("instruction: PUSH "); 
            printf_if_full(instr->if_full);
            printf_block(instr->block);
            break;
        case pull_instruction: 
            printf("instruction: PULL "); 
            printf_if_empty(instr->if_empty);
            printf_block(instr->block);
            break;
        case mov_instruction: 
            printf("instruction: MOV "); 
            printf_destination(instr->destination);
            printf_operation(instr->operation);
            printf_source(instr->source);
            break;
        case irq_instruction: 
            printf("instruction: IRQ "); 
            printf_clear(instr->clear);
            printf_wait(instr->wait);
            break;
        case set_instruction: 
            printf("instruction: SET "); 
            printf_destination(instr->destination);
            printf_value(instr->index_or_value);
            break;
        case empty_instruction: printf("instruction: no instruction! "); break;
    };
    printf_side_set_value(instr->side_set_value);
    printf_delay(instr->delay);
    printf_address(instr->address);
    printf("sm: %d ", instr->executing_sm_num);
    printf("\n");
}

void printf_instructions() {
    int i,p;
    printf("\nINSTRUCTIONS: \n");
    FOR_ENUMERATION(pio, pio_t, hardware_pio) {
      printf("pio: %d (%d)\n", p, pio->next_instruction_location);
      for (i = 0; i<pio->next_instruction_location; i++) {
        printf("  ");
        printf_instruction(&(pio->instructions[i]));        
      }
    }
}

static void printf_user_instruction(user_instruction_t* instr) {
    printf("Line: %d ", instr->line);
    switch (instr->instruction_type) {
        case write_instruction: 
            printf("instruction: WRITE "); 
            printf_value(instr->value);
            break;
        case read_instruction: 
            printf("instruction: READ "); 
            printf("VAR_NAME: %s ", instr->var_name);
            break;
        case pin_instruction: 
            printf("instruction: PIN ");
            printf("%d ", instr->pin);
            if (instr->set_high) printf("=> HIGH; ");
            else printf("=> LOW; ");
            break;
        case user_print_instruction: 
            printf("print %s ", instr->var_name);
            break;
        case data_instruction: 
            switch (instr->data_operation_type) {
                case data_write:
                    printf("data read ");
                    break;
                case data_read:
                    printf("data read ");
                    break;
                case data_readln:
                    printf("data readln ");
                    break;
                case data_print:
                    printf("data print ");
                    break;
                case data_clear:
                    printf("data clear ");
                    break;
                case data_set:
                    printf("data set ");
                    break;
                case no_data_operation:
                    printf("data but no operation? ");
                    break;
                };
                break;
            case repeat_instruction:
                printf("repeat ");
                break;
            case exit_instruction:
                printf("exit ");
                break;
        case empty_instruction: 
                printf("instruction: no instruction! "); 
                break;
    };
    printf_delay(instr->delay);
    printf_address(instr->address);
    if (instr->continue_user) printf("will continue ");
    printf("\n");
}

void printf_user_instructions() {
    int unum,i,p;
    printf("\nUSER INSTRUCTIONS: \n");
    unum = 0;
    FOR_ENUMERATION(up, user_processor_t, hardware_user_processor) {
      unum++;
      printf("up: %d  PC: %d next_instruction_location: %d\n", unum, up->pc, up->next_instruction_location);
      for (i = 0; i<up->next_instruction_location; i++) {
        printf("  ");
        printf_user_instruction(&(up->instructions[i]));        
      }
    }
}

void printf_defines() {
    int i;
    printf("\nDEFINES: \n");
    FOR_ENUMERATION(def, define_t, instruction_defines) {
        if (def->defined) {
            printf(" define: %s => %2d\n", def->symbol, def->value);
        }
    }
}

void printf_labels() {
    int i;
    printf("\nLABELS: \n");
    for (i=0; i<instruction_num_labels(); i++) printf("label: %s => %2d\n", instruction_label_symbol(i), instruction_label_location(i));
}

void printf_hardware_configuration() {
    printf("\nHardware Configuration:\n");
    printf("   Current PIO: %d", hardware_pio_num_set());
    printf("   Current SM: %d\n", hardware_sm_num_set());
    FOR_ENUMERATION(pio, pio_t, hardware_pio) {
        printf("   pio: %d\n", pio->this_num);
        FOR_ENUMERATION(sm, sm_t, hardware_sm) {
            if (sm->pio_num == pio->this_num) {
                printf("      sm: %d\n", sm->this_num);
                printf("         program name: %s\n", sm->program_name);
                printf("         pc: %d\n", sm->pc);
                printf("         first pc: %d\n", sm->first_pc);
                printf("         in   pins base: %d\n", sm->in_pins_base);
                printf("         out  pins base: %d  num_pins: %d\n", sm->out_pins_base, sm->out_pins_num);
                printf("         set  pins base: %d  num_pins: %d\n", sm->set_pins_base, sm->set_pins_num);
                printf("         sset pins base: %d  num_pins: %d  optional: %d\n", sm->side_set_pins_base, sm->side_set_pins_num, sm->side_set_pins_optional);
                printf("         shift out dir: %d\n", sm->shiftctl_out_shiftdir);
            }
        }
    }
    printf("Devices Enabled: \n");
    FOR_ENUMERATION(device, hardware_device_t, hardware_device_enumerator) {
        if (device->enabled) printf("    %s\n", device->name);
    }    
}

/**********************************************************************************
 * prints 
 **********************************************************************************/

static void print_side_set_value(int8_t side_set_value) {
    if (side_set_value > 0) status_msg("side set value: %2d  ", side_set_value);   
}
                                     
static void print_delay(uint8_t delay_value) {
    status_msg("delay value: %2d  ", delay_value);   
}
                                     
static void print_jmp_condition(condition_e condition) {
    switch (condition) {
        case always: status_msg("always       "); break;
        case x_zero: status_msg("if x is zero "); break;
        case y_zero: status_msg("if y is zero "); break;
        case x_decrement: status_msg("if x is not zero, decrement and jump "); break;
        case y_decrement: status_msg("if y is not zero, decrement and jump "); break;
        case x_not_equal_y: status_msg("if x != y    "); break;
        case pin_condition: status_msg("if pin indicated by EXECCTRL_JMP_PIN is high "); break;
        case not_osre: status_msg("if OSRE is not empty "); break;
        case unset_condition: status_msg("unset! "); 
    };
    status_msg("\n");
}
                                     
static void print_polarity(bool p) {
    if (p) {status_msg("polarity: one ");}
    else {status_msg("polarity: zero ");}
}
                                     
static void print_wait_source(wait_source_e w) {
    switch(w) {
        case gpio_source: status_msg("wait source: gpio "); break;
        case pin_source: status_msg("wait_source: pin "); break;
        case irq_source: status_msg("wait_source: irq "); break;
        case unset_wait_source: status_msg("wait source not set ! "); break;
    };
}
                                     
static void print_source(source_e s) {
    switch (s) {
        case pins_source: status_msg("source: pins selected by PINCTRL_IN_BASE + bit count "); break;
        case x_source: status_msg("source: x scratch register "); break;
        case y_source: status_msg("source: y scratch register "); break;
        case null_source: status_msg("source: null "); break;
        case isr_source: status_msg("source: isr "); break;
        case osr_source: status_msg("source: osr "); break;
        case status_source: status_msg("source: status as specified by EXECCTRL_STATUS_SEL "); break;
        case unset_source: status_msg("source is not set ! "); break;
    };
}
                                     
static void print_destination(destination_e d) {
    switch (d) {
        case pins_destination: status_msg("destination: pins selected by PINCTRL_IN_BASE + bit count "); break;
        case x_destination: status_msg("destination: x scratch register "); break;
        case y_destination: status_msg("destination: y scratch register "); break;
        case null_destination: status_msg("destination: null "); break;
        case pindirs_destination: status_msg("destination: pindirs "); break;
        case pc_destination: status_msg("destination: instruction counter "); break;
        case isr_destination: status_msg("destination: isr "); break;
        case exec_destination: status_msg("destinatino: EXEC "); break;
        case unset_destination: status_msg("destination is not set ! "); break;
    };
}

static void print_if_full(bool iff) {
    if (iff) {status_msg("if_full true "); ;}
    else {status_msg("if_full false ");}
}
                                     
static void print_if_empty(bool iff) {
    if (iff) {status_msg("if_empty true "); ;}
    else {status_msg("if_empty false ");}
}

static void print_block(bool bl) {
    if (bl) {status_msg("instruction will block/wait ");}
    else {status_msg("instruction will not block/wait ");}
}
                                     
static void print_operation(operation_e op) {
    switch (op) {
        case no_operation: status_msg("operation: none "); break;
        case invert: status_msg("operation: invert "); break;
        case bit_reverse: status_msg("operation: bit reverse "); break;
        case clear_operation: status_msg("operation: clear "); break;
        case wait_operation: status_msg("operatino: wait "); break;
        case unset_operation: status_msg("operation is not set !"); break;
    };
}
                                     
static void print_index(uint8_t iov) {
    status_msg("index: %2d ", iov);
}
                                     
static void print_value(uint32_t iov) {
    status_msg("value: %08x ", iov);
}
                                     
static void print_bit_count(uint8_t sc) {
    status_msg("shift count: %2d ", sc);
}
                                     
static void print_clear(bool cl) {
    if(cl) {status_msg("clear: true ");}
    else {status_msg("clear: false ");}
}
                                     
static void print_wait(bool w) {
    if(w) {status_msg("wait: true ");}
    else {status_msg("wait: false ");}
}
                                     
static void print_location(uint8_t sc) {
    status_msg("location: %s(%2d) -> %2d ", instruction_label_symbol(sc), sc, instruction_label_location(sc));
}

static void print_address(int addr) {   
   status_msg("address: %2d ", addr);
}

static void print_jmp_pc(uint8_t addr) {   
   status_msg("jmp pc: %2d ", addr);
}

void print_instruction(instruction_t* instr) {
    status_msg("Line: %d ", instr->line);
    switch (instr->instruction_type) {
        case jmp_instruction: 
            status_msg("instruction: JMP "); 
            print_jmp_condition(instr->condition);
            print_jmp_pc(instr->jmp_pc);
            break;
        case wait_instruction: 
            status_msg("instruction: WAIT "); 
            print_polarity(instr->polarity);
            print_wait_source(instr->wait_source);
            print_index(instr->index_or_value);
            break;
        case nop_instruction: 
            status_msg("instruction: NOP "); 
            break; 
        case in_instruction: 
            status_msg("instruction: IN "); 
            print_source(instr->source);
            print_bit_count(instr->bit_count);
            break;
        case out_instruction: 
            status_msg("instruction: OUT "); 
            print_destination(instr->destination);
            print_bit_count(instr->bit_count);
            break;
        case push_instruction: 
            status_msg("instruction: PUSH "); 
            print_if_full(instr->if_full);
            print_block(instr->block);
            break;
        case pull_instruction: 
            status_msg("instruction: PULL "); 
            print_if_empty(instr->if_empty);
            print_block(instr->block);
            break;
        case mov_instruction: 
            status_msg("instruction: MOV "); 
            print_destination(instr->destination);
            print_operation(instr->operation);
            print_source(instr->source);
            break;
        case irq_instruction: 
            status_msg("instruction: IRQ "); 
            print_clear(instr->clear);
            print_wait(instr->wait);
            break;
        case set_instruction: 
            status_msg("instruction: SET "); 
            print_destination(instr->destination);
            print_value(instr->index_or_value);
            break;
        case empty_instruction: status_msg("instruction: no instruction! "); break;
    };
    print_side_set_value(instr->side_set_value);
    print_delay(instr->delay);
    print_address(instr->address);
    status_msg("\n");
}

void set_print_lines(char * filename){
  FILE* fp;
  char ch;
  int  saw_cr = 0;
  int  saw_lf = 0;
  int  line_start = 0;
  bool saw_eof = false;
  int  num_chars = 0;
  int  num_lines = 0;
  if (!filename) {
      printf("no filename!\n");
      exit(-1);
  }
  fp = fopen(filename, "r");
  if (!fp) {
    printf("unable to open file %s", filename);
    exit(-2);
  }
 for (num_chars=0, ch=fgetc(fp); (num_chars < TEXT_BUFFER_SIZE) && !saw_eof; num_chars++, ch=fgetc(fp)) {
    print_line_buffer[num_chars] = ch;
    // account for Windows and Linux style EOLs - either CRLF or just LF
    if (saw_lf) {
      if (saw_cr && saw_lf) print_line_ends[num_lines] = num_chars - 2;
      else print_line_ends[num_lines] = num_chars - 1;
      print_line_starts[num_lines] = line_start;
      if (++num_lines >= MAX_NUMBER_OF_LINES) {
        printf("ERROR: file size exceeds max number of lines!!!\n");
        break;
      }
      line_start = num_chars;  /* next line starts here */
      saw_lf = 0;
      saw_cr = 0;
    }
    if (ch == '\r') saw_cr = 1;
    if (ch == '\n') saw_lf = 1;
    if (ch == EOF) saw_eof = true;
  }
  if (num_chars == TEXT_BUFFER_SIZE) printf("ERROR: file size exceeds buffer size!!!\n");
  /* handle last line that is unhandled when loop ends */
  if (saw_lf && saw_cr) print_line_ends[num_lines] = num_chars - 2;
  else print_line_ends[num_lines] = num_chars - 1;
  print_line_starts[num_lines++] = line_start;
  for (int i=0; i<num_lines; i++) print_line(i);
}



void print_line(int line_num) {
  int i;
  int len;
  printf("%2d ", line_num); 
  line_num--;
  len = print_line_ends[line_num] - print_line_starts[line_num];
  for (i=print_line_starts[line_num]; i < (print_line_starts[line_num] + len); i++) {
    putc(print_line_buffer[i],stdout);
  }
  printf("\n");
}