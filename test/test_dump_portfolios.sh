#!/bin/bash

ME=$(basename $0)
BINDIR=./bin
#VALGRIND='/usr/local/bin/valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=/tmp/valgrind_output.%p --gen-suppressions=all --suppressions=./util/valgrind_supression.sup'
#VALGRIND='/usr/local/bin/valgrind --tool=massif --stacks=yes --time-unit=ms --massif-out-file=/tmp/massif_output.%p'
VALGRIND=

QUERY="${VALGRIND} $BINDIR/query_portfolios"

if [ $# -ne 1 ]; then
  echo "Invalid number of arguments"
  echo "Usage: ${ME} <repetitions>"
  exit 254
fi

REPS=$1
DEFAULT_PORTFOLIOS=100
DEFAULT_ACCOUNTS=50
EXPECTED=$(echo "${REPS} * ${DEFAULT_ACCOUNTS} + ${DEFAULT_PORTFOLIOS}" | bc -l)

echo "*** Setup:"
echo "    Num repetitions: ${REPS}"
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
echo "Program is: ${QUERY} -a"
for i in $(seq 1 ${REPS}); do 
  ${QUERY} -a > /tmp/cnt$i 2>&1 &
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


