/*!
 * @file /print.h
 * @brief Simpio print utilties
 * @details
 * This provide functions to print instructions and hardware information that will either print to the UI screen or the terminal,
 * depending on how itis configured. It provides macros that can be used throughout Simpio to print the right level of 
 * information (basic, info, detail) without regard to whether it is being run in terminal/intreractive mode or UI.
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef INSTRUCTION_PRINT_H
#define INSTRUCTION_PRINT_H

#include "instruction.h"
#include "ui.h"

void printf_zero_pattern(uint32_t n, bool direction);

void printf_instructions();
void printf_user_instructions();
void printf_ih_instructions();
void printf_defines();
void printf_labels();
void printf_user_vars();

void printf_hardware_configuration();

void print_instruction(instruction_t * instr);

extern bool print_ui;
extern int  print_level;

void set_print_ui(bool ui);
void set_print_level(int level);

void set_print_lines(char * filename);
void print_line(int line);

#define DEBUG_PRINT_LEVEL 2
#define INFO_PRINT_LEVEL 1
#define MIN_PRINT_LEVEL 0

#define PRINT(...) if (print_ui) { status_msg(__VA_ARGS__); } else { printf(__VA_ARGS__); }
#define PRINTI(...) if (print_level>MIN_PRINT_LEVEL) { PRINT(__VA_ARGS__) }
#define PRINTD(...) if (print_level>INFO_PRINT_LEVEL) { PRINT(__VA_ARGS__) }

#endif