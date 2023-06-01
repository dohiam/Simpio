#~/bin/bash
#!
#  @file /run_tests.sh
#  @brief Tests all pio files in the current directory
#  @details
#  Gets a list of pio files in the current directory and passes it to another script
#  to run each test through simpio.
#  
#   fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
# 
set -x
ls -1 *.pio | xargs ./run_test.sh 
