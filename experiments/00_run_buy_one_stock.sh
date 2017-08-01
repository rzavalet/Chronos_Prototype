#!/bin/bash

BINDIR=../bin

START=$(date +%s.%N)
${BINDIR}/purchase_stock -s 0 -u 1 -p 1000 -c 1 > /dev/null 2>&1
END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)
echo ${DIFF}
