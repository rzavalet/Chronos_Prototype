#!/bin/bash

#==========================================================
# The objective of this test is to verify  that we can update
# a given row multiple times by multiple processes.
#==========================================================

ME=$(basename $0)
BINDIR=./bin

#VALGRIND='/usr/local/bin/valgrind --tool=memcheck --leak-check=yes --track-origins=yes --log-file=/tmp/valgrind_output.%p --gen-suppressions=all --suppressions=./util/valgrind_supression.sup'
#VALGRIND='/usr/local/bin/valgrind --tool=massif --stacks=yes --time-unit=ms --massif-out-file=/tmp/massif_output.%p'
VALGRIND=

PROGRAM="${VALGRIND} $BINDIR/update_stocks"

if [ $# -ne 2 ]; then
  echo "Invalid number of arguments"
  echo "Usage: ${ME} <repetitions> <rows_per_repetition>"
  exit 254
fi

REPS=$1
ROWS=$2

echo "*** Setup:"
echo "    Num repetitions: ${REPS}"
echo "    Num updates per repetition: ${ROWS}"
echo ""

#----------------------------------------
# Do the initial cleanup: delete all output
# files from previous runs
#----------------------------------------
echo "*** Initial cleanup"
rm -rf /tmp/upd* /tmp/valgrind_output* /tmp/massif*
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

#----------------------------------------
# Perform the initial load of the database
#----------------------------------------
echo "*** Performing initial load"
${PROGRAM} -l > /tmp/upd0 2>&1
if [ $? -ne 0 ]; then
  echo "FAILED: Error performing initial load"
  exit 254
fi

#----------------------------------------
# Now let's spawn multiple processes that
# update a random row. Since we don't have
# control over the updated row, this test
# is a bit random.
# All these processes are executed
# concurrently.
#----------------------------------------
echo "*** Starting test"
echo "*** Running loop"
echo "Program is: ${PROGRAM} -c ${ROWS}"
for i in $(seq 1 $REPS); do 
  ${PROGRAM} -c ${ROWS} > /tmp/upd$i 2>&1 &
done

wait

#----------------------------------------
# Once the program finished, let's count
# the number of successful transactions.
# We do so by searching a given message
# in each process' log file.
# We should find exactly ${NUM_PROCESS} of
# those messages. Otherwise, it means that
# some process wasn't able to update all
# its rows
#----------------------------------------
echo "*** Checking output"
for i in $(seq 1 $NUM_PROCESS); do 
  cnt=$(grep "Attempted transactions: ${ROWS}, Successful transactions: ${ROWS}, Failed transactions: 0" /tmp/upd$i  | wc -l)
  if [ $cnt -ne 1 ]; then
    echo "FAILED: Wrong count in file: /tmp/upd${i}"
    exit 253
  fi
done

echo "PASSED"
exit 0



