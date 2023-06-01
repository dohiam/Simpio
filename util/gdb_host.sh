#!/bin/bash
#!
#  @file /gdb_host.sh
#  @brief utility to help debug UI issues
#  @details
#  When built with debug symbols, this utility will help debug UI issues.
#  This host sesson will contain the gdb shell and connects to the server
#  session which will be the one actually the running simpio UI screen.
#  
#   fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
# 

gdb -ex 'target remote localhost:1234' simpio