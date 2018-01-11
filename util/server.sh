#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Invalid number of parameters"
  exit 255
fi

CLIENTS=$1
LOAD=$2
#PORT=$2
#MODE=$3

FILEOUT=/tmp/server.out
#FILESAMPLE=/tmp/sample.out

echo "** Starting server"
#../bin/startup_server -c $CLIENTS -s 5 -p $PORT -n -m $MODE > $FILEOUT 2>&1

if [ $LOAD -eq 1 ]; then
../bin/startup_server -m 0 -r 60 -c $CLIENTS -u 1 -v 10000 2>&1 | tee $FILEOUT
else
../bin/startup_server -m 0 -r 60 -c $CLIENTS -u 1 -v 10000 -n 2>&1 | tee $FILEOUT
fi

#grep -P 'SAMPLING' $FILEOUT | head -n 12 > $FILESAMPLE ; perl sample2csv.pl $FILESAMPLE
