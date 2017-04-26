#!/bin/bash

CHRONOS_DIR=/home/ricardozavaleta/Chronos_Prototype
CHRONOS_BIN_DIR=${CHRONOS_DIR}/bin
CHRONOS_SERVER_BIN=${CHRONOS_BIN_DIR}/startup_server
CHRONOS_CLIENT_BIN=${CHRONOS_BIN_DIR}/startup_client

CHRONOS_SERVER_ADDRESS="127.0.0.1"
CHRONOS_PORT=12345
CHRONOS_MODE_BASE=0
CHRONOS_MODE_AC=1
CHRONOS_MODE_AUP=2

CHRONOS_SERVER_OUTPUT=server_output.txt
CHRONOS_CLIENT_OUTPUT=client_output.txt

DURATION=15

#-----------------------
# Setup the enviroment
#-----------------------
make -f CHRONOS_DIR init

#=================================================================
# Experiment 1:
# We spawn a chronos server and two chronos clients with the following 
# characteristics
#=================================================================

#------------------------------------
# Server:
# - Up to 350 concurrent threads
# - TCP connection queue: 1024
# - fvi for data items is 1[s]
# - 100 update threads. Each update thread pulls 30 prices
# - p for data items is 0.5[s]
# - \beta is 2
# - Desired delay bound is 3[s]
# - experiment lasts 15 minutes
#------------------------------------
CLIENT_THREADS=350
UPDATE_THREADS=100
TCP_CONNECTION_QUEUE=1024
FLEXIBLE_VALIDITY_INTERVAL=1000
UPDATE_PERIOD=500
DESIRED_DELAY_BOUND=3
BETA=2
SAMPLING_PERIOD=30

${CHRONOS_SERVER_BIN} -c ${CLIENT_THREADS} -v ${FLEXIBLE_VALIDITY_INTERVAL} -u ${UPDATE_THREADS} -r ${DURATION} -t ${UPDATE_PERIOD} -s ${SAMPLING_PERIOD} -p ${CHRONOS_PORT} -m ${CHRONOS_MODE_BASE} > ${CHRONOS_SERVER_OUTPUT} 2>&1  &
sleep 30


#------------------------------------
# Client:
# - From 300 to 900 threads
# - Thinking time, 0.3-0.5
# - experiment lasts 15 minutes
#------------------------------------
CLIENT_THREADS_2=300
THINK_TIME=300
PERCENTAGE_VIEW_TXN=60

${CHRONOS_CLIENT_BIN} -c ${CLIENT_THREADS_2} -t ${THINK_TIME} -r ${DURATION} -a ${CHRONOS_SERVER_ADDRESS} -p ${CHRONOS_PORT} -v ${PERCENTAGE_VIEW_TXN} > ${CHRONOS_CLIENT_OUTPUT}.0 2>&1 &
${CHRONOS_CLIENT_BIN} -c ${CLIENT_THREADS_2} -t ${THINK_TIME} -r ${DURATION} -a ${CHRONOS_SERVER_ADDRESS} -p ${CHRONOS_PORT} -v ${PERCENTAGE_VIEW_TXN} > ${CHRONOS_CLIENT_OUTPUT}.1 2>&1 &

wait

