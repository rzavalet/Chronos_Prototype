#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Invalid number of parameters"
  exit 255
fi

CLIENTS=$1
#PORT=$2

FILEOUT=/tmp/client.out

echo "** Starting client"

#../bin/startup_client -c $CLIENTS -p $PORT
#../bin/startup_client -c $CLIENTS 2>&1 | tee $FILEOUT
gdb --args ../bin/startup_client -c $CLIENTS -d 0
