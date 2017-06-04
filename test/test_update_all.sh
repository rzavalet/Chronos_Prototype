#!/bin/bash

#==========================================================
# The objective of this test is to verify  that we can update
# all stock rows multiple times
#==========================================================
ME=$(basename $0)
BINDIR=./bin

#VALGRIND='/usr/local/bin/valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=/tmp/valgrind_output.%p --gen-suppressions=all --suppressions=./util/valgrind_supression.sup'
#VALGRIND='/usr/local/bin/valgrind --tool=massif --stacks=yes --time-unit=ms --massif-out-file=/tmp/massif_output.%p'
VALGRIND=

UPDATE="${VALGRIND} $BINDIR/update_stocks"
QUERY="${VALGRIND} $BINDIR/query_stocks"

if [ $# -ne 1 ]; then
  echo "Invalid number of arguments"
  echo "Usage: ${ME} <repetitions>"
  exit 254
fi

REPS=$1
NUM_QUOTES=10

echo "*** Setup:"
echo "    Num repetitions: ${REPS}"
echo ""

#----------------------------------------
# Do the initial cleanup: delete all output
# files from previous runs
#----------------------------------------
echo "*** Initial cleanup"
rm -rf /tmp/cnt* /tmp/upd* /tmp/valgrind_output* /tmp/massif*
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi


echo "*** Performing initial load"
${UPDATE} -l > /tmp/upd0 2>&1
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

#---------------------------------------
# Now lets run multiple processes sequentially
# In each run, we update all rows with the same value.
# At the end of each run, we verify that all updates
# were successful
#---------------------------------------
echo "*** Entering update loop"
for i in $(seq 1 $REPS); do
    UPDATE_VALUE=$RANDOM
    
    ${UPDATE} -a -v ${UPDATE_VALUE} > /tmp/upd${i} 2>&1
    cnt=$(grep 'Attempted transactions: 10, Successful transactions: 10, Failed transactions: 0' /tmp/upd$i  | wc -l)
    if [ $cnt -ne 1 ]; then
	echo "FAILED: Wrong count"
	exit 253
    fi

    ${QUERY} -a > /tmp/cnt${i} 2>&1
    cnt=$(grep -P "Value of \w+ is ${UPDATE_VALUE}.000000" /tmp/cnt${i}  | wc -l)
    if [ $cnt -ne $NUM_QUOTES ]; then
	echo "FAILED: Wrong count"
	exit 252
    fi
done

echo "PASSED"
exit 0




