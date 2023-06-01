/*!
 * @file /parser.h
 * @brief Functions for executing the Simpio program file parser
 * @details
 * For invoking the parser with and without debugging info. The implementation is in simpio.y (yacc grammar file with embedded actions
 * for adding instructions and updating hardware configuration.
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef SIMPIO_H
#define SIMPIO_H

int simpio_parse(char * pio_file_name);

void simpio_parse_debug(char * pio_file_name);

extern int yydebug;

#endif