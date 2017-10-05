#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Invalid number of parameters"
  exit 255
fi

CLIENTS=$1
PORT=$2

../bin/startup_client -c $CLIENTS -p $PORT
