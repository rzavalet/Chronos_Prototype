#!/bin/bash

UTILSDIR=./util
OUTPUT=/tmp/test.out
EXPECTED=1100

echo "*** Running test"
sh $UTILSDIR/query_stocks_loop.sh > $OUTPUT 2>&1 
if [ $? -ne 0 ]; then
  echo "FAILED: Error running script"
  exit 254
fi

echo "*** Checking output"
count=$(grep Symbol $OUTPUT | wc -l)

if [ $count -ne $EXPECTED ]; then
  echo "FAILED: count: $count expected: $EXPECTED"
  exit 255
else
  echo "PASSED"
  exit 0
fi


