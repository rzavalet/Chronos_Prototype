#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Invalid number of parameters"
  exit 255
fi

CLIENTS=$1
PORT=$2
MODE=$3

FILEOUT=/tmp/server.out
FILESAMPLE=/tmp/sample.out
CLIENTOUT=/tmp/client.out

echo "** Starting server"
../bin/startup_server -c $CLIENTS -s 5 -p $PORT -n -m $MODE > $FILEOUT 2>&1

grep -P 'SAMPLING' $FILEOUT | head -n 12 > $FILESAMPLE ; perl sample2csv.pl $FILESAMPLE
