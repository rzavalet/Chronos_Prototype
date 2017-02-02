#ifndef _STARTUP_SERVER_H_
#define _STARTUP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
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

#define CHRONOS_SERVER_CTX_MAGIC      (0xBACA)
#define CHRONOS_SERVER_THREAD_MAGIC   (0xCACA)

#define CHRONOS_SERVER_CTX_CHECK(_ctxt)    assert((_ctxt)->magic == CHRONOS_SERVER_CTX_MAGIC)
#define CHRONOS_SERVER_THREAD_CHECK(_thr)    assert((_thr)->magic == CHRONOS_SERVER_THREAD_MAGIC)

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
  CHRONOS_SERVER_THREAD_PROCESSING,
  CHRONOS_SERVER_THREAD_MAX,
  CHRONOS_SERVER_THREAD_INVAL=CHRONOS_SERVER_THREAD_MAX
} chronosServerThreadType_t;

const char *chronosServerThreadNames[] ={
  "CHRONOS_SERVER_THREAD_LISTENER",
  "CHRONOS_SERVER_THREAD_UPDATE"
};

#define CHRONOS_SERVER_THREAD_NAME(_txn_type) \
  ((CHRONOS_SERVER_THREAD_MIN<=(_txn_type) && (_txn_type) < CHRONOS_SERVER_THREAD_MAX) ? (chronosServerThreadNames[(_txn_type)]) : "INVALID")

#define CHRONOS_SERVER_THREAD_IS_VALID(_txn_type) \
  (CHRONOS_SERVER_THREAD_MIN<=(_txn_type) && (_txn_type) < CHRONOS_SERVER_THREAD_MAX)

typedef struct chronosServerStats_t {
  chronos_time_t cumulative_time[CHRONOS_USER_TXN_MAX];
  int num_txns[CHRONOS_USER_TXN_MAX];

  chronos_time_t average_time[CHRONOS_USER_TXN_MAX];
} chronosServerStats_t;

typedef struct {
  char txn_type;
  chronos_time_t txn_start;
} txn_info_t;
  
#define BSIZE 1024
typedef struct {
#if 0
  char buf[BSIZE]; /* we actually store TXN identifiers -- see chronos.h */
#endif
  txn_info_t txnInfoArr[BSIZE];
  int occupied;
  int nextin;
  int nextout;
  unsigned long long ticketReq;
  unsigned long long ticketDone;
  pthread_mutex_t mutex;
  pthread_cond_t more;
  pthread_cond_t less;
  pthread_cond_t ticketReady;
} chronos_queue_t;


typedef struct chronosServerContext_t {

  int magic;
  /* We can only create a limited number of 
   * client threads. */
  int numClientsThreads;
  int currentNumClients;

  /* These two variables are used to wait till 
   * all client threads are initialized, so that
   * we can have a fair experiment*/
  pthread_mutex_t startThreadsMutex;
  pthread_cond_t startThreadsWait;

  /* Apart from client threads, we also have
   * update threads - threads that update the
   * data in the system periodically */
  int numUpdateThreads;

  int duration;
  
  struct timespec validityInterval;
  int serverPort;
  struct timespec updatePeriod;
  int runningMode;
  int debugLevel;
  chronosServerStats_t *stats;
  chronos_queue_t userTxnQueue;
  chronos_queue_t sysTxnQueue;

  /* These fields control the sampling task */
#define CHRONOS_SAMPLE_ARRAY_SIZE  (15*60*60)
  unsigned long long txn_count[CHRONOS_SAMPLE_ARRAY_SIZE];
  unsigned long long txn_timely[CHRONOS_SAMPLE_ARRAY_SIZE];
  unsigned long long txn_received[CHRONOS_SAMPLE_ARRAY_SIZE];
  unsigned long long txn_update[CHRONOS_SAMPLE_ARRAY_SIZE];
  unsigned long long txn_enqueued[CHRONOS_SAMPLE_ARRAY_SIZE];
  struct timespec txn_duration[CHRONOS_SAMPLE_ARRAY_SIZE];
  struct timespec txn_avg_duration[CHRONOS_SAMPLE_ARRAY_SIZE];
  struct timespec txn_delay[CHRONOS_SAMPLE_ARRAY_SIZE];
  struct timespec txn_avg_delay[CHRONOS_SAMPLE_ARRAY_SIZE];  
  int numSamples;
  int samplingPeriod;
  int currentSlot;
  struct timespec start;
  struct timespec end;
  timer_t timer_id;
  struct itimerspec timer_et;
  struct sigevent timer_ev;

  /* These fields are for the benchmark framework */
  void *benchmarkCtxtP;
  
} chronosServerContext_t;

typedef struct chronosServerThreadInfo_t {
  int       magic;
  pthread_t thread_id;
  int       thread_num;
  int       socket_fd;
  chronosServerThreadState_t state;
  chronosServerThreadType_t thread_type;
  chronosServerContext_t *contextP;
} chronosServerThreadInfo_t;

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

static void *
processThread(void *argP);

static int
dispatchTableFn (chronosRequestPacket_t *reqPacketP, chronosServerThreadInfo_t *infoP);

static int
waitPeriod(struct timespec updatePeriod);

static int
updateStats(chronos_user_transaction_t txn_type, chronos_time_t *new_time, chronosServerThreadInfo_t *infoP);

static int
performanceMonitor(chronos_time_t *begin_time, chronos_time_t *end_time, chronos_user_transaction_t txn_type, chronosServerThreadInfo_t *infoP);
#endif
