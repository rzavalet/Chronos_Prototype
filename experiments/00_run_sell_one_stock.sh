#!/bin/bash

BINDIR=../bin

START=$(date +%s.%N)
${BINDIR}/sell_stock -s 0 -u 1 -p 1 -c 1 > /dev/null 2>&1
END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)
echo ${DIFF}
