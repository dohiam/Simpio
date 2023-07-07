#!/usr/bin/python

import sys

if len(sys.argv) < 2:
    print("usage: " + sys.argv[0] + "<input_file> to generate a real pio file from a simpio pio program file\n")
    sys.exit()

inf = open(sys.argv[1], 'r')
if not inf:
    print("error opening file " + sys.argv[1] + "\n")
    
project_name = sys.argv[1].split('.')[0]
    
config_mapping = { 'JMP_PIN'             : 'sm_config_set_jmp_pin(&sm_config,<1>);',
                   'SET_PINS'            : 'sm_config_set_set_pins(&sm_config,<1>,<2>); \n    for (uint itmp=<1>; itmp < <1> + <2>; itmp++) pio_gpio_init(pio, itmp); \n    pio_sm_set_consecutive_pindirs(pio, sm, <1>, <2>, true);',
                   'IN_PINS'             : 'sm_config_set_in_pins(&sm_config,<1>);',
                   'OUT_PINS'            : 'sm_config_set_out_pins(&sm_config,<1>,<2>); \n    for (uint itmp=<1>; itmp < <1> + <2>; itmp++) pio_gpio_init(pio, itmp); \n    pio_sm_set_consecutive_pindirs(pio, sm, <1>, <2>, true);',
                   'SIDE_SET_PINS'       : 'sm_config_set_sideset_pins(&sm_config,<1>); \n    pio_gpio_init(pio, <1>);',
                   'SIDE_SET_COUNT'      : 'sm_config_set_sideset(&sm_config,<1>,<2>,<3>);',
                   'SHIFTCTL_OUT'        : 'sm_config_set_out_shift(&sm_config,<1>,<2>,<3>);',
                   'SHIFTCTL_IN'         : 'sm_config_set_in_shift(&sm_config,<1>,<2>,<3>);',
                   'EXECCTRL_STATUS_SEL' : 'sm_config_set_mov_status(&sm_config,<1>,<2>);',
                   'FIFO_MERGE'          : 'sm_config_set_fifo_join(&sm_config,<1>);',
                   'CLKDIV'              : 'sm_config_set_clkdiv_int_frac (&sm_config, <1>, 0);',
                   'USER_VAR'            : 'uint32_t <1>;',
                   'USER_PROCESSOR'      : '\n' }
                  

user_mapping = { 'WRITE'            : 'pio_sm_put_blocking(pio, sm, <1>);',
                 'READ'             : '<1> = pio_sm_get_blocking(pio, sm);',
                 'PRINT'            : 'printf("<1> = %08X\\n", <1>);',
                 'DATA READ'        : 'ch = pio_sm_get_blocking(pio, sm);',
                 'DATA READLN'      : 'receive_line(data, MAX_STRING, pio, sm);',
                 'DATA WRITE'       : 'send_string(data, pio, sm);',
                 'DATA SET'         : 'strncpy(data, <2>, MAX_STRING);',
                 'DATA PRINT'       : 'printf("%s\\n", data);',
                 'DATA CLEAR'       : "data[0] = '\\0';",
                 'EXIT'             : '',
                 'PIN'              : 'gpio_put(<1>, <2>);' }

user_program_headers = '#include <stdio.h> \n#include <string.h>\n#include <stdint.h>\n#include "pico/stdlib.h"\n#include "hardware/pio.h"\n#include "pico/multicore.h"\n#include "' + project_name + '.pio.h"' 

helper_functions = """

#define MAX_STRING 40

void send_string(char * s, PIO pio, uint sm) {
    int len = strlen(s);
    int i;
    printf("sending: ");
    for (i=0; i<len; i++) {
        printf("%c",s[i]);
        pio_sm_put_blocking(pio, sm, s[i]);
    }
    printf("\\n");
}

void receive_line(char * s, int max, PIO pio, uint sm) {
    int i;
    char ch = ' ';
    for (i=0; i<max-1 && ch != '.'; i++) {
        ch = pio_sm_get_blocking(pio, sm);
        s[i] = ch;
    }
    s[i] = '\\0';
}

"""

CMakeLists = """
cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(${myprojectname} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}.pio)
target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib hardware_pio pico_multicore)
pico_add_extra_outputs(${PROJECT_NAME})
"""

USB_SERIAL = """
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
"""
    
RS232_SERIAL = """
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
"""

multicore_launch_placeholder = "<<multicore_launch_placeholder>>"
    
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

from dataclasses import dataclass

@dataclass
class pio_program:
    name: str
    init_function: str
    pio: str
    sm: str
    
@dataclass
class user_program:
    name: str
    pio: str
    sm: str
    body: str
    up: int
    
#
# Main
#"

in_lines = inf.readlines()
serial = 'RS232'
cpp = -1
cup = -1
current_pio = 0
current_sm = 0
ppgms = []
upgms = []

pio_file_string = ''


line_num = 0
for line in in_lines:
    line_num = line_num+1
    words = line.split()
    if len(words) > 0 and words[0].upper() == 'DATA':
        words[0] = words[0] + ' ' + words[1]
    if len(words) > 0 and words[0].upper() == 'DATA SET':
        words[2] = ' '.join(words[2::])
    if len(words) > 0 and words[0].upper()== ".PROGRAM":
        cpp = cpp + 1
        program_name = words[1]
        ppgms.append(pio_program(name=program_name, init_function="% c-sdk {\nstatic inline void " + program_name + "_init() {", pio='', sm=''))
        pio_file_string = pio_file_string + line
        #user_program_headers = user_program_headers + '\n#include ' + program_name + '.pio.h'
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "PIO":
        if words[2] == "1":
            current_pio = "pio1"
        else:
            current_pio = "pio0"
        ppgms[cpp].pio = current_pio
        ppgms[cpp].init_function = ppgms[cpp].init_function + "\n" + "    PIO pio = " + current_pio + ";"
        ppgms[cpp].init_function = ppgms[cpp].init_function + "\n" + "    uint offset = pio_add_program(pio, &" + ppgms[cpp].name + "_program);"
        ppgms[cpp].init_function = ppgms[cpp].init_function + "\n" + "    pio_sm_config sm_config = " + ppgms[cpp].name + "_program_get_default_config(offset);"
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "SM":
        current_sm = words[2]
        ppgms[cpp].sm = current_sm
        ppgms[cpp].init_function = ppgms[cpp].init_function + "\n" + "    uint sm = " + current_sm + ";"
        ppgms[cpp].init_function = ppgms[cpp].init_function + "\n" + "    pio_sm_claim(pio, sm);"
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "SERIAL":
        serial = words[2]
        if serial.upper() == 'USB':
            CMakeLists = CMakeLists + USB_SERIAL
        else:
            CMakeLists = CMakeLists + RS232_SERIAL
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "USER_PROCESSOR":
        cup = cup + 1
        user_processor = words[2]
        if (user_processor == '0'):
            up = 0
            body = "int main() {"
        elif (user_processor == '1'):
            up = 1
            body = "void up1() {"
        else:
            print("ERROR: invalid user processor: " + user_processor + " line: " + str(line_num) + "\n");
            sys.exit(-1)        
        upgms.append(user_program(ppgms[cpp].name, ppgms[cpp].pio, ppgms[cpp].sm, body, up))
        upgms[cup].body = upgms[cup].body + "\n" + "    static char data[MAX_STRING];"
        upgms[cup].body = upgms[cup].body + "\n" + "    PIO pio = " + ppgms[cpp].pio + ";"
        upgms[cup].body = upgms[cup].body + "\n" + "    uint sm = " + ppgms[cpp].sm + ";"
        if user_processor == '0':
            upgms[cup].body = upgms[cup].body + '\n    stdio_init_all();'
            upgms[cup].body = upgms[cup].body + '\n    sleep_ms(5000);'
            upgms[cup].body = upgms[cup].body + '\n    printf("main starting\\n");'
            upgms[cup].body = upgms[cup].body + multicore_launch_placeholder
        upgms[cup].body = upgms[cup].body + '\n    ' + upgms[cup].name + '_init();'
    elif len(words) > 0 and words[0].upper() == ".CONFIG" and words[1].upper() == "USER_VAR":
        template = config_mapping[words[1].upper()]
        arg_list = words[2:]
        upgms[cup].body = upgms[cup].body + '\n    ' + subst_args(arg_list, template)
    elif len(words) > 0 and is_user_instruction(words[0]):
        template = user_mapping[words[0].upper()]
        arg_list = words[1:]
        upgms[cup].body = upgms[cup].body + "\n" + "    " + subst_args(arg_list, template)
    elif len(words) > 1 and is_config_instruction(words[1]):
        template = config_mapping[words[1].upper()]
        arg_list = words[2:]
        ppgms[cpp].init_function = ppgms[cpp].init_function + "\n" + "    " + subst_args(arg_list, template)
    else:
        pio_file_string = pio_file_string + line
                                                         
inf.close()

if len(upgms) > 1:
    multicore_launch = '\n    multicore_launch_core1(up1);'
else:
    multicore_launch = ''
for u in upgms:
    if u.up == 0:
        u.body = u.body.replace(multicore_launch_placeholder, multicore_launch)
        u.body = u.body + '\n    while (true) {\n        printf("main idling ...\\n");\n        sleep_ms(10000);\n    }'


for p in ppgms:
    p.init_function = p.init_function + "\n    pio_sm_init(pio, sm, offset, &sm_config);\n    pio_sm_set_enabled(pio, sm, true);\n}\n%}\n"

for u in upgms:
    u.body = u.body + "\n}"

print("\nCMakeLists.txt:")
print('set(myprojectname "' + project_name + '")')
pn=0
for p in ppgms:
      print('set(program' + str(pn) + ' "' + p.name + '")')
      pn = pn + 1
print(CMakeLists)

print("\nOUTPUT PROGRAM:")
print(pio_file_string)

for p in ppgms:
    print("\nINIT FUNCTION:")
    print(p.init_function)

print("\nUSER PROGRAM:")
print(user_program_headers)
print(helper_functions)
if len(upgms) > 1:
    print(upgms[1].body + '\n')
if len(upgms) > 0:
    print(upgms[0].body + '\n')

f = open("CMakeLists.txt", "w")
f.write('set(myprojectname "' + project_name + '")\n')
pn = 0
for p in ppgms:
      f.write('set(program' + str(pn) + ' "' + p.name + '")\n')
      pn = pn+1
f.write(CMakeLists)
f.close()

f = open(project_name + ".pio", "w")
f.write(pio_file_string)
for p in ppgms:
    f.write(p.init_function)
f.close()

f = open(project_name+".c", "w")
f.write(user_program_headers)
f.write(helper_functions)
if len(upgms) > 1:
    f.write(upgms[1].body)
if len(upgms) > 0:
    f.write(upgms[0].body)
f.close()
