#!/bin/bash

BINDIR=./bin
NUM_PROCESS=1
NUM_QUOTES=10

echo "*** Performing initial load"
$BINDIR/query_stocks -l > /tmp/load 2>&1
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running first update"
$BINDIR/update_stocks -a -v 0 > /tmp/upd$i 2>&1
cnt=$(grep 'Attempted transactions: 10, Successful transactions: 10, Failed transactions: 0' /tmp/upd$i  | wc -l)
if [ $cnt -ne 1 ]; then
  echo "FAILED: Wrong count"
  exit 253
fi

$BINDIR/query_stocks -a > /tmp/cnt 2>&1
cnt=$(grep -P 'Value of \w+ is 0.000000' /tmp/cnt  | wc -l)
if [ $cnt -ne $NUM_QUOTES ]; then
  echo "FAILED: Wrong count"
  exit 252
fi

echo "*** Running second update"
$BINDIR/update_stocks -a -v 100 > /tmp/upd$i 2>&1
cnt=$(grep 'Attempted transactions: 10, Successful transactions: 10, Failed transactions: 0' /tmp/upd$i  | wc -l)
if [ $cnt -ne 1 ]; then
  echo "FAILED: Wrong count"
  exit 253
fi

$BINDIR/query_stocks -a > /tmp/cnt 2>&1
cnt=$(grep -P 'Value of \w+ is 100.000000' /tmp/cnt  | wc -l)
if [ $cnt -ne $NUM_QUOTES ]; then
  echo "FAILED: Wrong count"
  exit 252
fi

echo "*** Running third update"
$BINDIR/update_stocks -a -v 200 > /tmp/upd$i 2>&1
cnt=$(grep 'Attempted transactions: 10, Successful transactions: 10, Failed transactions: 0' /tmp/upd$i  | wc -l)
if [ $cnt -ne 1 ]; then
  echo "FAILED: Wrong count"
  exit 253
fi

$BINDIR/query_stocks -a > /tmp/cnt 2>&1
cnt=$(grep -P 'Value of \w+ is 200.000000' /tmp/cnt  | wc -l)
if [ $cnt -ne $NUM_QUOTES ]; then
  echo "FAILED: Wrong count"
  exit 252
fi

echo "PASSED"
exit 0




