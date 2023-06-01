#!/bin/bash
#!
#  @file /gdb_server.sh
#  @brief utility to help debug UI issues
#  @details
#  When built with debug symbols, this script sets up a simpio debug session.
#  This server session will contain the UI being debugged.
#  
#   fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
# 

gdbserver localhost:1234 simpio $1
