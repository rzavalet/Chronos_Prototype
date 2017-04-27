#!/bin/bash

BINDIR=./bin

echo "*** Performing initial load"
$BINDIR/query_stocks -l
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running loop"
for i in $(seq 1 10); do 
  $BINDIR/query_stocks &
done

wait

echo "*** Done"
exit 0

