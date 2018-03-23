#!/bin/bash

CRLIBM=../crlibm/.libs/libcrlibm.so.0.0.0
NEWGLIBC=../glibc/build/glibc/math/libm.so

./time_math2 input-decks/exp-inputs || exit 1
./time_math2 -d input-decks/exp-inputs-hard || exit 2
./time_math2 -A $CRLIBM -a exp_rn  input-decks/exp-inputs-hard || exit 3
./time_math2 -f cos input-decks/cos-inputs || exit 4
./time_math2 -t -c 500 -i 1000  input-decks/exp-inputs-hard || exit 5
./time_math2 -f pow input-decks/pow-inputs || exit 6
./compare_vals -a exp_rn $CRLIBM input-decks/exp-inputs-hard
./compare_vals -f pow -a pow_rn $CRLIBM input-decks/pow-inputs
./compare_vals $NEWGLIBC input-decks/exp-inputs-hard
./compare_vals -1 $NEWGLIBC input-decks/exp-inputs-hard
./compare_vals -f pow $NEWGLIBC input-decks/pow-inputs 
./compare_vals -f pow -1 $NEWGLIBC input-decks/pow-inputs 

# Still not sure how to make this work
#./time_math2 -r -c 50 -i 1000 -s 5 input-decks/exp-inputs-hard || exit 5
