#!/bin/bash

# num=1
# var=$( cat ../examples/example${num}_command )
# echo $var
# ./cacheSim "../examples/example${num}_trace" ${var} > 1.txt
# gdb --args ./cacheSim "../examples/example${num}_trace" ${var}

RED=`tput setaf 1`
GREEN=`tput setaf 2`

echo "${CYAN}pray: "
for num in {1..20}
do
    var=$(cat ./tests/example${num}_command)
    ./cacheSim ${var} > ./tests/our_res.txt
    diff ./tests/our_res.txt ./tests/example${num}_output
    if cmp ./tests/our_res.txt ./tests/example${num}_output
        then
    echo "${GREEN}test ${num} passed";
        else
        echo "${RED}test ${num} failed";
    fi
done
rm ./tests/our_res.txt
echo

