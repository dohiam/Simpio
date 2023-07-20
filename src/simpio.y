%{
/*!
 * @file /simpio.y
 * @brief Simpio language grammar rules
 * @details
 * This is the yacc/bison implementation of the PIO intructions and configuration statements. 
 *
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"
#include "hardware.h"
#include "instruction.h"
#include "execution.h"
#include "ui.h"
#include "parser.h"
#include "print.h"
#include "devices.h"

extern FILE *yyin;
extern int yylineno;
extern int yylex();
extern int line_count;
int yyparse();

int yydebug=1;

instruction_t ci;         /* information associated with the current instruction */
user_instruction_t uci;   /* information associated with the current user instruction */
pio_t cpio;               /* information associated with the current pio */
program_t cprog;          /* information associated with the current program */

void yyerror(const char *str)
{
    PRINT("syntax error: %s\n", str);
}

int yywrap()
{
    return 1;
}

void system_init() {
    line_count = 0;
    instruction_set_defaults(&ci);
    hardware_set_system_defaults();  
}

int simpio_parse(char * pio_file_name)
{
    int rc;
    char ch;
    PRINTD("parsing %s\n", pio_file_name);
    system_init();
    yyin = fopen(pio_file_name, "r");
    rc = yyparse();
    fclose(yyin);
    if (rc != 0) return yylineno;
    rc = instruction_fix_forward_labels();
    if (rc > 0) {PRINT("Could not resolve label on line %d\n", rc);}
    else {PRINTD("All references found and fixed\n");}
    if (rc == 0) return 0;
    else return yylineno;
}

void simpio_parse_debug(char * pio_file_name)
{
    int rc;
    PRINTD("parsing %s\n", pio_file_name);
    system_init();
    yyin = fopen(pio_file_name, "r");
    rc = yyparse();
    fclose(yyin);
    if (rc != 0) { PRINT("error on line %d\n", yylineno); }
    else { PRINT("syntax check complete, all ok\n"); }
}

/*
int simpio_test_build(int argc, char** argv)
{
    char ch;
    int rc;
    output2stdout = 1;
    PRINTD("starting");
    system_init();
    yyin = fopen(argv[1], "r");
    rc = yyparse();
    fclose(yyin);
    if (!rc) {
        printf_hardware_configuration();
        printf_labels();
        printf_defines();
        rc = instruction_fix_forward_labels();
        if (rc > 0) printf("Error: could not resolve label on line %d\n", rc);
        printf_instructions();
        printf_user_instructions();
        return rc;
    }
    else return rc;    
}
*/

%}


%union {
    int       ival;
    char      sval[32];  
    char    strval[STRING_MAX];
}

%token _DEFINE _PROGRAM _ORIGEN _WRAP_TARGET _WRAP _LANG_OPT _WORD 
%token _JMP _NOP _WAIT _IN _OUT _PUSH _PULL _MOV _SET _PUBLIC  _OPT
%token _GPIO _IRQ _NOT_X _NOT_Y _X_DECREMENT _Y_DECREMENT _X_NOT_EQUAL_Y _PIN _NOT_OSRE _SIDE _EOL
%token _PINS _PINDIRS _X _Y _NULL _ISR _OSR _PC _EXEC
%token _IFFULL _IFEMPTY _BLOCK _NOBLOCK _CLEAR _NOWAIT _RELATIVE _STATUS
%token _AUTOPUSH _AUTOPULL
%token _BANG _COLON _COLON_COLON

%token _CONFIG _PIO _SM _PIN_CONDITION _SET_PINS _IN_PINS _OUT_PINS _SIDE_SET_PINS _SIDE_SET_COUNT _USER_PROCESSOR _FIFO_MERGE _CLKDIV _DATA_CONFIG _SERIAL _USB _RS232
%token _SHIFTCTL_OUT _SHIFTCTL_IN 
%token _DEVICE _SPI_FLASH

%token <ival> _BINARY_DIGIT _HEX_NUMBER _BINARY_NUMBER _DECIMAL_NUMBER _DELAY
%token <sval> _SYMBOL 
%token <strval> _STRING

%token _EXECCTRL_STATUS_SEL _TX_LEVEL _RX_LEVEL

%token _WRITE _READ _DATA _READLN _PRINT _REPEAT _EXIT _VAR _HIGH _LOW _CONTINUE_USER

%type <ival> expression mulexp primary number 

%%

/****************************************************************************************************************
 *  a program file is a list of statements which are either an empty/comment line, label, directive or instruction
 ****************************************************************************************************************/

statement_list: statement _EOL  { PRINTD("----------Done with line %d; going to next line----------\n", line_count+1); line_count++; } |  
                statement_list statement _EOL { PRINTD("----------Done with line %d; going to next line----------\n", line_count+1); line_count++; } ;

statement:  /* empty */ | label | directive | instruction_side {  ci.line = line_count+1; if(!instruction_add(&ci)) {yylineno--; return -1;} instruction_set_defaults(&ci); } | 
                                              user_instruction_continue { uci.line = line_count+1; if (!instruction_user_add(&uci)) {yylineno--; return -1;} instruction_set_user_defaults(&uci); };

label: _SYMBOL _COLON { PRINTD("label: %s\n", $1); instruction_add_label($1); } ;

/****************************************************************************************************************
 *  directives: program, origen, side_set, opt_pindirs, wrap, lang_opt, and word
 ****************************************************************************************************************/
 
directive: define_directive | program_directive | origen_directive | wrap_target_directive | wrap_directive | lang_opt_directive | word_directive | config_directive | data_directive | device_directive;

config_directive: _CONFIG config_statement

config_statement: _PIO number { hardware_set_pio($2, line_count); } | _SM number { hardware_set_sm($2, line_count); } | 
                  _PIN_CONDITION number { hardware_set_pin_condition($2); } |
                  _SET_PINS number number { hardware_set_set_pins($2,$3, line_count); } | 
                  _IN_PINS number { hardware_set_in_pins($2, line_count); } | 
                  _OUT_PINS number number { hardware_set_out_pins($2,$3, line_count); } |
                  _SIDE_SET_PINS number {hardware_set_side_set_pins($2, line_count); } |
                  _SIDE_SET_COUNT number number number { hardware_set_side_set_count($2, $3, $4, line_count); } |
                  _SHIFTCTL_OUT number number number { hardware_set_shiftctl_out($2, $3, $4, line_count); } |
                  _SHIFTCTL_IN number number number { hardware_set_shiftctl_in($2, $3, $4, line_count); } |
                  _EXECCTRL_STATUS_SEL number number {hardware_set_status_sel($2, $3); } |
                  _USER_PROCESSOR number{hardware_set_up($2, line_count);} |
                  _FIFO_MERGE number { hardware_fifo_merge($2); } |
                  _VAR _SYMBOL { instruction_var_define($2); } |
                  _SERIAL _RS232 | _SERIAL _USB |
                  _CLKDIV number;

data_directive: _DATA_CONFIG _STRING { hardware_set_data($2); }

define_directive: _DEFINE _SYMBOL expression { PRINTD("%s = %d", $2, $3); instruction_add_define($2, $3, line_count); } 

program_directive: _PROGRAM _SYMBOL { hardware_set_program_name($2); } 

origen_directive: _ORIGEN expression /* TODO */

wrap_target_directive: _WRAP_TARGET

wrap_directive: _WRAP

lang_opt_directive: _LANG_OPT _SYMBOL  { /* todo */ }

word_directive:  _WORD expression { /* todo */ }

device_directive: _DEVICE _SPI_FLASH number number number number { devices_enable_spi_flash($3, $4, $5, $6); }

/****************************************************************************************************************
 * instructions: 
 ****************************************************************************************************************/
 
instruction_side: instruction {ci.side_set_value= -1; ci.delay=0;} | instruction _SIDE number { ci.side_set_value = $3;  ci.delay=0;} |
                  instruction _DELAY {ci.side_set_value= -1; ci.delay = $2; } |  
                  instruction _SIDE number _DELAY { ci.side_set_value = $3;  ci.delay = $4; } ;

instruction:  jmp_instruction | nop_instruction | wait_instruction | in_instruction | out_instruction | push_instruction | pull_instruction | mov_instruction | irq_instruction | set_instruction;

user_instruction_continue: user_instruction_delay _CONTINUE_USER { uci.continue_user = true; } | user_instruction_delay { uci.continue_user = false; } ;

user_instruction_delay:  user_instruction _DELAY {uci.delay = $2; } | user_instruction {uci.delay = 0;};

user_instruction: write_instruction | read_instruction | data_instruction | print_instruction | repeat_instruction | pin_instruction | exit_instruction;

jmp_instruction: _JMP jmp_condition _SYMBOL { ci.instruction_type = jmp_instruction;  snprintf(ci.label, SYMBOL_MAX, "%s", $3); ci.location = instruction_find_label(ci.label);  
                                              if (ci.location==NO_LOCATION) {PRINTI("Note: label %s not found (on first pass)", $3);} } 

jmp_condition:  /* empty */ { ci.condition = always; } | _NOT_X { ci.condition = x_zero; } | _NOT_Y { ci.condition = y_zero; } | _X_DECREMENT { ci.condition = x_decrement; } | 
                _Y_DECREMENT { ci.condition = y_decrement; } | _X_NOT_EQUAL_Y { ci.condition = x_not_equal_y; } | _PIN { ci.condition = pin_condition; } | _NOT_OSRE { ci.condition = not_osre; } ;

nop_instruction: _NOP { ci.instruction_type = nop_instruction; } 

wait_instruction: _WAIT polarity wait_source { ci.instruction_type = wait_instruction; } ;

polarity: _BINARY_DIGIT { ci.polarity = $1; } ; 

wait_source: _GPIO index { ci.wait_source = gpio_source; } | _PIN index { ci.wait_source = pin_source; } | _IRQ index relative { ci.wait_source = irq_source; } 

index: expression { ci.index_or_value = $1; }

relative: /* empty */ { ci.is_relative = false; } | _RELATIVE  { ci.is_relative = true; } ;

in_instruction: _IN source expression { ci.instruction_type = in_instruction; ci.bit_count = $3;} ;

source: _PINS {ci.source = pins_source; } | _X {ci.source = x_source; } | _Y {ci.source = y_source; } | _NULL {ci.source = null_source; } | 
        _ISR {ci.source = isr_source; } | _OSR {ci.source = osr_source; } | _STATUS { ci.source = status_source; } ;

out_instruction: _OUT destination expression { ci.instruction_type = out_instruction; ci.bit_count = $3;  } 

destination: _PINS { ci.destination = pins_destination; } | _X { ci.destination = x_destination; } | _Y { ci.destination = y_destination; } | _NULL { ci.destination = null_destination; } | 
             _PINDIRS { ci.destination = pindirs_destination; } | _PC { ci.destination = pc_destination; } | _ISR { ci.destination = isr_destination; } | 
             _OSR { ci.destination = osr_destination; } | _EXEC { ci.destination = exec_destination; }

push_instruction: _PUSH  iffull_block { ci.instruction_type = push_instruction; } ;

iffull_block: /* empty */ {ci.if_full = 0; ci.block = false; } | _IFFULL { ci.if_full = 1; ci.block = true; } | 
              _IFFULL _NOBLOCK { ci.if_full = 1; ci.block = 0; } | _IFFULL _BLOCK { ci.if_full = 1; ci.block = 1; } |
              _NOBLOCK { ci.if_full = 0; ci.block = 0; } | _BLOCK { ci.if_full = 1; ci.block = 1; } ;

pull_instruction: _PULL ifempty_block { ci.instruction_type = pull_instruction; } 

ifempty_block: /* empty */ {ci.if_empty = 0; ci.block = true; } | _IFEMPTY { ci.if_empty = 1; ci.block = true; } | 
              _IFEMPTY _NOBLOCK { ci.if_empty = 1; ci.block = 0; } | _IFEMPTY _BLOCK { ci.if_empty = 1; ci.block = 1; } |
              _NOBLOCK { ci.if_empty = 0; ci.block = 0; } | _BLOCK { ci.if_empty = 1; ci.block = 1; } ;

mov_instruction: _MOV destination operation source { ci.instruction_type = mov_instruction; } 

operation: /* empty */ { ci.operation = no_operation; } | _BANG { ci.operation = invert; } | _COLON_COLON {ci.operation = bit_reverse; }

irq_instruction: _IRQ irq_operation number relative { ci.instruction_type = irq_instruction; ci.index_or_value = $3; } 

irq_operation: /* empty */ | _SET { ci.operation = set_operation; } | _NOWAIT { ci.operation = nowait_operation; } | _WAIT { ci.operation = wait_operation; } | _CLEAR { ci.operation = clear_operation; } ;

set_instruction: _SET  destination expression { ci.instruction_type = set_instruction; ci.index_or_value = $3; } ;

write_instruction: _WRITE number { uci.delay = 0; uci.instruction_type = write_instruction; uci.value = $2; } ;

read_instruction: _READ _SYMBOL { uci.delay = 0; uci.instruction_type = read_instruction; snprintf(uci.var_name, SYMBOL_MAX, "%s", $2); } ;

print_instruction: _PRINT _SYMBOL { uci.delay = 0; uci.instruction_type = user_print_instruction; snprintf(uci.var_name, SYMBOL_MAX, "%s", $2); } ;

data_instruction: _DATA _WRITE  { uci.delay = 0; uci.instruction_type = data_instruction; uci.data_operation_type = data_write; } |
                  _DATA _READ   { uci.delay = 0; uci.instruction_type = data_instruction; uci.data_operation_type = data_read; } |
                  _DATA _READLN { uci.delay = 0; uci.instruction_type = data_instruction; uci.data_operation_type = data_readln; } |
                  _DATA _PRINT  { uci.delay = 0; uci.instruction_type = data_instruction; uci.data_operation_type = data_print; } |
                  _DATA _CLEAR  { uci.delay = 0; uci.instruction_type = data_instruction; uci.data_operation_type = data_clear; } |
                  _DATA _SET  _STRING  { uci.delay = 0; uci.instruction_type = data_instruction; uci.data_operation_type = data_set; instruction_add_data(&uci, $3); } ;

repeat_instruction: _REPEAT { uci.delay = 0; uci.instruction_type = repeat_instruction; } ;

exit_instruction: _EXIT { uci.delay = 0; uci.instruction_type = exit_instruction; } ;

pin_instruction: _PIN number _HIGH { uci.delay = 0; uci.instruction_type = pin_instruction; uci.pin = $2; uci.set_high = true; } |
                 _PIN number _LOW  { uci.delay = 0; uci.instruction_type = pin_instruction; uci.pin = $2; uci.set_high = false; } ;

/* typical expression grammar to imply unary minus is highest precedence, then multiply and divide, and lastly plus and minus TODO: not sure if pioasm allows parens in expressions */

expression: expression '+' mulexp { $$ = $1 + $3; } | expression '-' mulexp { $$ = $1 - $3; } | mulexp { $$ = $1; } ;

mulexp: mulexp '*' primary { $$ = $1 * $3; } | mulexp '/' primary { if ($3==0) $$ = 0; else $$ = $1 / $3; } | primary { $$ = $1; } ;

primary: '(' expression ')' { $$ = $2; } | '-' primary { $$ = - $2; }| number { $$ = $1; };

number: _BINARY_DIGIT { $$ = $1; } | _BINARY_NUMBER { $$ = $1; } | _HEX_NUMBER { $$ = $1; } | _DECIMAL_NUMBER { $$ = $1; }  ;


