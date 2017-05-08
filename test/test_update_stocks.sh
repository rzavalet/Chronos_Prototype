#!/bin/bash

BINDIR=./bin
NUM_PROCESS=100

echo "*** Performing initial load"
$BINDIR/query_stocks -l > /tmp/load 2>&1
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running loop"
for i in $(seq 1 $NUM_PROCESS); do 
  $BINDIR/update_stocks > /tmp/upd$i 2>&1 &
done

wait

for i in $(seq 1 $NUM_PROCESS); do 
  cnt=$(grep 'Attempted transactions: 100, Successful transactions: 100, Failed transactions: 0' /tmp/upd$i  | wc -l)
  if [ $cnt -ne 1 ]; then
    echo "FAILED: Wrong count"
    exit 253
  fi
done

echo "PASSED"
exit 0



