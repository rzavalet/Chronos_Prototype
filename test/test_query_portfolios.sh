#!/bin/bash

ME=$(basename $0)
BINDIR=./bin
#VALGRIND='/usr/local/bin/valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=/tmp/valgrind_output.%p --gen-suppressions=all --suppressions=./util/valgrind_supression.sup'
#VALGRIND='/usr/local/bin/valgrind --tool=massif --stacks=yes --time-unit=ms --massif-out-file=/tmp/massif_output.%p'
VALGRIND=

QUERY="${VALGRIND} $BINDIR/query_portfolios"

if [ $# -ne 2 ]; then
  echo "Invalid number of arguments"
  echo "Usage: ${ME} <repetitions> <rows_per_repetition>"
  exit 254
fi

REPS=$1
ROWS=$2
DEFAULT_ACCOUNTS=100
EXPECTED=$(echo "${REPS} * ${ROWS} + ${DEFAULT_ACCOUNTS}" | bc -l)

echo "*** Setup:"
echo "    Num repetitions: ${REPS}"
echo "    Num updates per repetition: ${ROWS}"
echo ""

echo "*** Initial cleanup"
rm -rf /tmp/cnt* /tmp/valgrind_output* /tmp/massif*
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Performing initial load"
${QUERY} -l > /tmp/cnt0 2>&1
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

echo "*** Running test"
echo "*** Running loop"
echo "Program is: ${QUERY} -c ${ROWS}"
for i in $(seq 1 ${REPS}); do 
  ${QUERY} -c ${ROWS} > /tmp/cnt$i 2>&1 &
done

wait

echo "*** Checking output"
echo "Expecting ${EXPECTED} rows read"
count=$(grep 'AccountId' /tmp/cnt* | wc -l)

if [ $count -ne $EXPECTED ]; then
  echo "FAILED: count: $count expected: $EXPECTED"
  exit 255
fi

echo "PASSED"
exit 0


