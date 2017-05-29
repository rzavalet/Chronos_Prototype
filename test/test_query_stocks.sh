#!/bin/bash

ME=$(basename $0)
BINDIR=./bin
#VALGRIND='/usr/local/bin/valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=/tmp/valgrind_output.%p --gen-suppressions=all --suppressions=./util/valgrind_supression.sup'
VALGRIND='/usr/local/bin/valgrind --tool=massif --stacks=yes --time-unit=ms --massif-out-file=/tmp/massif_output.%p'
#PROGRAM="${VALGRIND} $BINDIR/query_stocks"
PROGRAM=$BINDIR/query_stocks

if [ $# -ne 2 ]; then
  echo "Invalid number of arguments"
  echo "Usage: ${ME} <repetitions> <rows_per_repetition>"
  exit 254
fi

REPS=$1
ROWS=$2
EXPECTED=$(echo "${REPS} * ${ROWS} + ${ROWS}" | bc -l)

echo "*** Initial cleanup"
rm -rf /tmp/cnt* /tmp/valgrind_output* /tmp/massif*
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Performing initial load"
${PROGRAM} -l -c ${ROWS} > /tmp/cnt0 2>&1
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running test"
echo "*** Running loop"
echo "Program is: ${PROGRAM} -c ${ROWS}"
for i in $(seq 1 ${REPS}); do 
  ${PROGRAM} -c ${ROWS} > /tmp/cnt$i 2>&1 &
done

wait

echo "*** Checking output"
echo "Expecting ${EXPECTED} rows read"
count=$(grep 'Value of' /tmp/cnt* | wc -l)

if [ $count -ne $EXPECTED ]; then
  echo "FAILED: count: $count expected: $EXPECTED"
  exit 255
else
  echo "PASSED"
  exit 0
fi


