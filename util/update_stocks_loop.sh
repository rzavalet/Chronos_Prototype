#!/bin/bash

BINDIR=./bin
NUM_PROCESS=3

echo "*** Performing initial load"
$BINDIR/query_stocks -l
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running loop"
for i in $(seq 1 $NUM_PROCESS); do 
  #$BINDIR/update_stocks 2>&1 | tee /tmp/upd$i &
  $BINDIR/update_stocks > /tmp/upd$i 2>&1 &
done

wait

echo "*** Done"
exit 0


