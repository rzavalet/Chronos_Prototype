#!/bin/bash

liblitmus=/home/ricardo/working_dir/liblitmus
rootdir=..
builddir=$rootdir/bin
homedir=$rootdir/databases

echo "Setting scheduler..."
${liblitmus}/setsched GSN-EDF
${liblitmus}/showsched

while getopts ":n:" opt
do
  case $opt in
    n) 
      echo "-n was triggered, Parameter: $OPTARG" >&2
      NUM_TXN=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done 

echo "Spawning processes...."
for i in `seq 1 ${NUM_TXN}`
do
	${liblitmus}/rt_launch -w 3000 3000 -- ${builddir}/view_stock_txn -h ${homedir}/  &
done
sleep 1

cat /proc/litmus/stats

${liblitmus}/release_ts
wait

echo "Unsetting scheduler..."
${liblitmus}/setsched Linux
${liblitmus}/showsched
