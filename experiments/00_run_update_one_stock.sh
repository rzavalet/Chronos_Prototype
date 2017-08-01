#!/bin/bash

BINDIR=../bin

START=$(date +%s.%N)
${BINDIR}/update_stocks -c 1 > /dev/null 2>&1
END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)
echo ${DIFF}
