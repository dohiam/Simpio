#~/bin/bash
#!
#  @file /run_test.sh
#  @brief Runs a list of test files, breaking on the last statement
#  @details
#  Input arguments are a space delimimited list of test files. For each file, 
#  a breakpoint is passed which is the number of lines in the file, i.e., 
#  the last statement in the file.
#  
#   fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
# 
set -x
for fn in "$@"
do
    nl=$(wc -l < ${fn})
    rc=$(./simpio ts ${fn} ${nl})
    echo "Finished ${fn}"
done

