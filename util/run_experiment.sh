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

echo "** Cleaning working directory"
make -C ../ init

echo "** Starting server"
../bin/startup_server -c $CLIENTS -s 5 -p $PORT -m $MODE > $FILEOUT 2>&1 &
PID_SERVER=$!

sleep 20

echo "** Starting client"
../bin/startup_client -c $CLIENTS -p $PORT > $CLIENTOUT 2>&1 &
PID_CLIENT=$!

echo "** Waiting for experiment to complete"
wait $PID_SERVER

echo "** Killing client"
kill $PID_CLIENT

echo "** Obtaining stats"
grep -P 'SAMPLING' $FILEOUT | head -n 12 > $FILESAMPLE

echo "## RESULTS FOR NUM CLIENTS = $CLIENTS"
perl sample2csv.pl $FILESAMPLE
echo "-------------------------------"


