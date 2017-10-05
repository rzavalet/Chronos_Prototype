#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Invalid number of parameters"
  exit 255
fi

MODE=$1

PORT=5000

for CLIENTS in `seq 5 5 50`; do 
  echo "CLIENTS: $CLIENTS - PORT: $PORT - MODE: $MODE"
  sh ./run_experiment.sh $CLIENTS $PORT $MODE
  PORT=$(( PORT + 50 ))
done
