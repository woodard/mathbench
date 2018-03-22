#!/bin/bash

./time_math2 input-decks/exp-inputs || exit 1
./time_math2 input-decks/exp-inputs-hard || exit 2
./time_math2 -r input-decks/exp-inputs-hard || exit 3
