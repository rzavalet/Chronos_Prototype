#!/bin/bash

ROOTDIR=${PWD}/..
TESTSDIR=${PWD}
RESULTSDIR=/tmp

#=======================
# Setup stuff
#=======================
cd $ROOTDIR
#make reinit
#make load
#make run_refresh_quotes
#make run_populate_portfolios

#=======================
# view_stock test
#=======================
TESTNAME="view_stock"
echo "Running test: $TESTNAME"
make ${TESTNAME} > $RESULTSDIR/${TESTNAME}.out 2>&1
diff $TESTSDIR/${TESTNAME}.mas $RESULTSDIR/${TESTNAME}.out > $RESULTSDIR/${TESTNAME}.diff
if [ -s $RESULTSDIR/${TESTNAME}.diff ]
then
  >&2 echo "Test Failed"
  exit 1
else
  echo "Test passed"
fi
