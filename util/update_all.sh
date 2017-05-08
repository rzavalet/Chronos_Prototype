#!/bin/bash

BINDIR=./bin
NUM_PROCESS=1

echo "*** Performing initial load"
$BINDIR/query_stocks -l
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running loop"
for i in $(seq 1 $NUM_PROCESS); do 
  $BINDIR/update_stocks -a -v 0
done

echo "*** Showing records"
$BINDIR/query_stocks
if [ $? -ne 0 ]; then
  echo "FAILED: Error showing stocks"
  exit 254
fi

echo "*** Done"
exit 0



