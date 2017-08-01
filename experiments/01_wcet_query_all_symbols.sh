#!/bin/bash

BINDIR=../bin
REPS=100
FILE=/tmp/stats

rm ${FILE}.*

# First execute the query multiple times:
for i in $(seq 1 $REPS); do 
  sh 03_run_query_all_symbols.sh > ${FILE}.${i}
done

# Print durations
cat ${FILE}.*
