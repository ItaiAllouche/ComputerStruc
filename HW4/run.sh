#!/bin/bash

RED=`tput setaf 1`
GREEN=`tput setaf 2`
CYAN=`tput setaf 6`

echo "${CYAN}Running tests..: "
for num in {1..20}
do
    var0=$( echo tests/example${num}.img)
    ./sim_main ${var0} > tests/example${num}.YoursOut
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./dflow_calc ${var0} ${var1} ${var2}  > ./valgrind_tests/this_year_leak${num}.txt 2>&1
    if cmp tests/example${num}.YoursOut tests/example${num}.out
        then
    echo "${GREEN}test ${num} passed";
        else
        echo "${RED}test ${num} failed";
    fi
done
echo