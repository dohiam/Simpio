#!/usr/bin/python

import sys

if len(sys.argv) < 1:
    print("usage: " + sys.argv[0] + "<input_file> to generate a real pio file from a simpio pio program file\n")
    sys.exit()

inf = open(sys.argv[1], 'r')
if not inf:
    print("error opening file " + sys.argv[1] + "\n")
    
config_mapping = { 'JMP_PIN'             : 'sm_config_set_jmp_pin(&sm_config,<1>);',
                   'SET_PINS'            : 'sm_config_set_set_pins(&sm_config,<1>,<2>);',
                   'IN_PINS'             : 'sm_config_set_in_pins(&sm_config,<1>);',
                   'OUT_PINS'            : 'sm_config_set_out_pins(&sm_config,<1>,<2>); \n    for (uint itmp=<1>; itmp < <1> + <2>; itmp++) pio_gpio_init(pio, itmp); \n    pio_sm_set_consecutive_pindirs(pio, sm, <1>, <2>, true);',
                   'SIDE_SET_PINS'       : 'sm_config_set_sideset_pins(&sm_config,<1>);',
                   'SIDE_SET_COUNT'      : 'sm_config_set_sideset(&sm_config,<1>,<2>,<3>);',
                   'SHIFTCTL_OUT'        : 'sm_config_set_out_shift(&sm_config,<1>,<2>,<3>);',
                   'SHIFTCTL_IN'         : 'sm_config_set_out_shift(&sm_config,<1>,<2>,<3>);',
                   'EXECCTRL_STATUS_SEL' : 'sm_config_set_mov_status(&sm_config,<1>,<2>);',
                   'FIFO_MERGE'          : 'sm_config_set_fifo_join(&sm_config,<1>);',
                   'CLKDIV'              : 'sm_config_set_clkdiv_int_frac (&sm_config, <1>, 0);',
                   'USER_PROCESSOR'      : '\n' }
                  

user_mapping = { 'WRITE'            : 'pio_sm_put_blocking(pio, sm, <1>);',
                 'READ'             : '<1> = pio_sm_get_blocking(pio, sm);',
                 'PIN'              : 'gpio_put(<1>, <2>);' }

def is_in_list(instr, instr_list):
    if instr.upper().strip() in instr_list:
        return True
    else:
        return False

def is_user_instruction(instr):
    return is_in_list(instr, user_mapping.keys())

def is_config_instruction(instr):
    return is_in_list(instr, config_mapping.keys())

def subst_args(arg_list, template):
    arg_num = 1
    result = template
    for arg in arg_list:
        to_replace = "<" + str(arg_num) + ">"
        result = result.replace(to_replace, arg)
        arg_num = arg_num + 1
    return result

#
# Main
#"

CMakeLists = """
cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${myprojectname} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT_NAME} blink.c)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}.pio)
target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(${PROJECT_NAME})
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
"""
    
in_lines = inf.readlines()
init_function = ""
pio_program = ""
user_program = '#include "pico/stdlib.h"\n#include "hardware/pio.h"'
program_name = "UNDEFINED"
pio = "pio0"
sm = 0

for line in in_lines:
    words = line.split()
    if len(words) > 0 and words[0].upper()== ".PROGRAM":
        program_name = words[1]
        init_function = init_function + "% c-sdk {\nstatic inline void " + program_name + "_init() {"
        pio_program = pio_program + line
        user_program = user_program + '\n#include "' + program_name + '.pio.h"\nint main() {' 
        user_program = user_program + "\n    " + program_name + "_init();"
        CMakeLists = 'set(myprojectname "' + program_name + '")\n' + CMakeLists
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "PIO":
        if words[2] == "1":
            pio = "pio1"
        else:
            pio = "pio0"
        init_function = init_function + "\n" + "    PIO pio = " + pio + ";"
        init_function = init_function + "\n" + "    uint offset = pio_add_program(pio, &" + program_name + "_program);"
        init_function = init_function + "\n" + "    pio_sm_config sm_config = " + program_name + "_program_get_default_config(offset);"
        user_program = user_program + "\n" + "    PIO pio = " + pio + ";"
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "SM":
        sm = words[2]
        init_function = init_function + "\n" + "    uint sm = " + sm + ";"
        init_function = init_function + "\n" + "    pio_sm_claim(pio, sm);"
        user_program = user_program + "\n" + "    uint sm = " + sm + ";"
    elif len(words) > 0 and is_user_instruction(words[0]):
        template = user_mapping[words[0].upper()]
        arg_list = words[1:]
        user_program = user_program + "\n" + "    " + subst_args(arg_list, template)
    elif len(words) > 1 and is_config_instruction(words[1]):
        template = config_mapping[words[1].upper()]
        arg_list = words[2:]
        init_function = init_function + "\n" + "    " + subst_args(arg_list, template)
    else:
        pio_program = pio_program + line
                                                         
inf.close()
            
init_function = init_function + "\n    pio_sm_init(pio, sm, offset, &sm_config);\n    pio_sm_set_enabled(pio, sm, true);\n}\n%}\n"
user_program = user_program + "\n}"
                
print("\nCMakeLists.txt:")
print(CMakeLists)
print("\nOUTPUT PROGRAM:")
print(pio_program)
print("\nINIT FUNCTION:")
print(init_function)
print("\nUSER PROGRAM:")
print(user_program)



f = open("CMakeLists.txt", "w")
f.write(CMakeLists)
f.close()

f = open(program_name + ".pio", "w")
f.write(pio_program)
f.write("\n")
f.write(init_function)
f.close()

f = open(program_name+".c", "w")
f.write(user_program)
f.close()
