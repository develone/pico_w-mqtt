#!/bin/bash

cd pi_tcp_tests

rm -f cli1 cli2 cli5 cli6

gcc -v client.c -Drem1  -o cli1
gcc -v client.c -Drem2  -o cli2


gcc -v client.c -Drem5  -o cli5
gcc -v client.c -Drem6  -o cli6