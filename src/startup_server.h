#ifndef _STARTUP_SERVER_H_
#define _STARTUP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "chronos.h"
#include "chronos_packets.h"
#include "chronos_config.h"
#include "benchmark.h"

int benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;
int chronos_debug_level = CHRONOS_DEBUG_LEVEL_MIN;

typedef enum chronosServerThreadState_t {
  CHRONOS_SERVER_THREAD_STATE_MIN = 0,
  CHRONOS_SERVER_THREAD_STATE_RUN = CHRONOS_SERVER_THREAD_STATE_MIN,
  CHRONOS_SERVER_THREAD_STATE_DIE,
  CHRONOS_SERVER_THREAD_STATE_MAX,
  CHRONOS_SERVER_THREAD_STATE_INVAL=CHRONOS_SERVER_THREAD_STATE_MAX
} chronosServerThreadState_t;

typedef enum chronosServerThreadType_t {
  CHRONOS_SERVER_THREAD_MIN = 0,
  CHRONOS_SERVER_THREAD_LISTENER = CHRONOS_SERVER_THREAD_MIN,
  CHRONOS_SERVER_THREAD_UPDATE,
  CHRONOS_SERVER_THREAD_MAX,
  CHRONOS_SERVER_THREAD_INVAL=CHRONOS_SERVER_THREAD_MAX
} chronosServerThreadType_t;

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
typedef int (*chronos_system_init_fn)(char *homedir, char *datafilesdir);
typedef int (*chronos_txn_handler_fn)(void *);
#endif

typedef struct chronosServerStats_t {
  chronos_time_t cumulative_time[CHRONOS_USER_TXN_MAX];
  int num_txns[CHRONOS_USER_TXN_MAX];

  chronos_time_t average_time[CHRONOS_USER_TXN_MAX];
} chronosServerStats_t;

typedef struct chronosServerContext_t {
  int numClientsThreads;
  int numUpdateThreads;
  int validityInterval;
  int serverPort;
  int updatePeriod;
  int runningMode;
  int currentNumClients;
  int debugLevel;
  chronosServerStats_t *stats;
  
#ifdef CHRONOS_NOT_YET_IMPLEMENTED
  chronos_system_init_fn  chronos_system_init;
  chronos_txn_handler_fn  chronos_txn_handler[CHRONOS_USER_TXN_MAX];
#endif
  
} chronosServerContext_t;

typedef struct chronosServerThreadInfo_t {
  pthread_t thread_id;
  int       thread_num;
  int       socket_fd;
  chronosServerThreadState_t state;
  chronosServerThreadType_t thread_type;
  chronosServerContext_t *contextP;
} chronosServerThreadInfo_t;

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
chronos_queue_t userTransactionsQueue;
chronos_queue_t updateTransactionsQueue;
#endif

static int
processArguments(int argc, char *argv[], chronosServerContext_t *contextP);

static void
chronos_usage();

static void *
daListener(void *argP);

static void *
daHandler(void *argP);

static void *
updateThread(void *argP);

static int
dispatchTableFn (chronosRequestPacket_t *reqPacketP, chronosServerThreadInfo_t *infoP);

static int
waitPeriod(int updatePeriod);

static int
getElapsedTime(chronos_time_t *begin, chronos_time_t *end, chronos_time_t *result);

static int
getCurrentTime(chronos_time_t *time);

static int
updateStats(chronos_user_transaction_t txn_type, chronos_time_t *new_time, chronosServerThreadInfo_t *infoP);

static int
printStats(chronosServerThreadInfo_t *infoP);

static int
performanceMonitor(chronos_time_t *begin_time, chronos_time_t *end_time, chronos_user_transaction_t txn_type, chronosServerThreadInfo_t *infoP);
#endif
