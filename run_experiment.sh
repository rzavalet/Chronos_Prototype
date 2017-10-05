#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Invalid number of parameters"
  exit 255
fi

CLIENTS=$1
PORT=$2
MODE=$3

FILEOUT=myfile.txt
FILESAMPLE=mysample.txt
CLIENTOUT=myclient.txt

echo "** Starting server"
./bin/startup_server -c $CLIENTS -u 50 -s 5 -p $PORT -n -m $MODE > $FILEOUT 2>&1 &
PID_SERVER=$!

sleep 10

echo "** Starting client"
./bin/startup_client -c $CLIENTS -p $PORT > $CLIENTOUT 2>&1 &
PID_CLIENT=$!

echo "** Waiting for experiment to complete"
wait $PID_SERVER

echo "** Killing client"
kill $PID_CLIENT

echo "** Obtaining stats"
grep -P 'SAMPLING' $FILEOUT > $FILESAMPLE

echo "## RESULTS FOR NUM CLIENTS = $CLIENTS"
perl util/sample2csv.pl $FILESAMPLE | head -n 13
echo "-------------------------------"


