#ifndef _STARTUP_CLIENT_H_
#define _STARTUP_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chronos.h"
#include "chronos_packets.h"
#include "benchmark.h"

int benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;
int chronos_debug_level = CHRONOS_DEBUG_LEVEL_MIN;

typedef struct chronosClientStats_t {
  int txnCount[CHRONOS_USER_TXN_MAX];
} chronosClientStats_t;

typedef struct chronosClientContext_t {
  char serverAddress[256];
  int serverPort;
  int numTransactions;
  double duration_sec;
  int percentageViewStockTransactions;
  int debugLevel;

  /* We can only create a limited number of 
   * client threads. */
  int numClientsThreads;

  double thinkingTime;
  
  char **stocksListP;

  int (*timeToDieFp)(void);

} chronosClientContext_t;

typedef struct chronosClientThreadInfo_t {
  pthread_t thread_id;
  int       thread_num;
  int       socket_fd;
  chronosClientStats_t stats;
  chronosClientContext_t *contextP;
} chronosClientThreadInfo_t;

/*========================================================
                        PROTOTYPES
=========================================================*/
static int
waitClientThreads(int num_threads, chronosClientThreadInfo_t *infoP, chronosClientContext_t *contextP);

static void
printClientStats(int num_threads, chronosClientThreadInfo_t *infoP, chronosClientContext_t *contextP);

static int
spawnClientThreads(int num_threads, chronosClientThreadInfo_t **info_ret, chronosClientContext_t *contextP);

static int
connectToServer(chronosClientThreadInfo_t *infoP);

static int
disconnectFromServer(chronosClientThreadInfo_t *infoP);

static void *
userTransactionThread(void *argP);

static int
pickTransactionType(chronos_user_transaction_t *txn_type_ret, chronosClientThreadInfo_t *infoP);

static int
waitTransactionResponse(chronos_user_transaction_t txnType, chronosClientThreadInfo_t *infoP);

static void
chronos_usage();

#endif
