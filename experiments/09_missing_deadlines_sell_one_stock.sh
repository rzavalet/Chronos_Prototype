#!/bin/bash

BINDIR=../bin
MAXSIZE=100
STEP=10
FIRST=20
FILE=/tmp/stats

for CURSIZE in $(seq ${FIRST} ${STEP} ${MAXSIZE}); do

# echo "Running with CURSIZE=${CURSIZE}"
# Clean old files
  rm ${FILE}.*

  ${BINDIR}/purchase_stock -s 0 -u 1 -p 1000 -c 10000 > /dev/null 2>&1

# First execute the query multiple times:
  for i in $(seq 1 ${CURSIZE}); do 
    sh 00_run_sell_one_stock.sh > ${FILE}.${i} &
  done

# Wait for tasks to finish
  wait

# Print durations
  CNT=0
  WCET=0.0062
  MAX=0
  #echo "#### Counting missed deadlines ####"
  for i in ${FILE}.*; do
    DUR=$(cat $i)
    #echo "DUR=${DUR}"
    #echo "WCET=${WCET}"
    if [ 1 -eq "$(echo "${DUR} > ${WCET}" | bc)" ]; then
      CNT=$((CNT+1))
    fi
    if [ 1 -eq "$(echo "${DUR} > ${MAX}" | bc)" ]; then
      MAX=${DUR}
    fi
  done

  echo -e "${CURSIZE} \t${CNT} \t${MAX}"
  #echo ""
done
