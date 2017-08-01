#!/bin/bash

BINDIR=../bin

START=$(date +%s.%N)
${BINDIR}/query_stocks -a > /dev/null 2>&1
END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)
echo ${DIFF}
