#!/bin/bash

BINDIR=../bin
REPS=100
FILE=/tmp/stats

rm ${FILE}.*

${BINDIR}/purchase_stock -s 0 -u 1 -p 1000 -c $REPS > /dev/null 2>&1
${BINDIR}/purchase_stock -s 0 -u 1 -p 1000 -c $REPS > /dev/null 2>&1

# First execute the query multiple times:
for i in $(seq 1 $REPS); do 
  sh 00_run_sell_one_stock.sh > ${FILE}.${i}
done

# Print durations
cat ${FILE}.*
