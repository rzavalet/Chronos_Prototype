#ifndef _CHRONOS_SERVER_H_
#define _CHRONOS_SERVER_H_

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "chronos.h"
#include "chronos_config.h"
#include "chronos_transactions.h"
#include "benchmark.h"

#define CHRONOS_SERVER_CTX_MAGIC      (0xBACA)
#define CHRONOS_SERVER_THREAD_MAGIC   (0xCACA)

#define CHRONOS_SERVER_CTX_CHECK(_ctxt)    assert((_ctxt)->magic == CHRONOS_SERVER_CTX_MAGIC)
#define CHRONOS_SERVER_THREAD_CHECK(_thr)    assert((_thr)->magic == CHRONOS_SERVER_THREAD_MAGIC)

#define CHRONOS_MODE_BASE   (0)
#define CHRONOS_MODE_AC     (1)
#define CHRONOS_MODE_AUP    (2)
#define CHRONOS_MODE_FULL   (3)

#define IS_CHRONOS_MODE_BASE(_ctxt)   ((_ctxt)->runningMode == CHRONOS_MODE_BASE)
#define IS_CHRONOS_MODE_AC(_ctxt)     ((_ctxt)->runningMode == CHRONOS_MODE_AC)
#define IS_CHRONOS_MODE_AUP(_ctxt)    ((_ctxt)->runningMode == CHRONOS_MODE_AUP)
#define IS_CHRONOS_MODE_FULL(_ctxt)   ((_ctxt)->runningMode == CHRONOS_MODE_FULL)

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

#define CHRONOS_SERVER_THREAD_NAME(_txn_type) \
  ((CHRONOS_SERVER_THREAD_MIN<=(_txn_type) && (_txn_type) < CHRONOS_SERVER_THREAD_MAX) ? (chronosServerThreadNames[(_txn_type)]) : "INVALID")

#define CHRONOS_SERVER_THREAD_IS_VALID(_txn_type) \
  (CHRONOS_SERVER_THREAD_MIN<=(_txn_type) && (_txn_type) < CHRONOS_SERVER_THREAD_MAX)

typedef struct chronosServerStats_t 
{
  int            num_txns;
  double         cumulative_time_ms;

  int            num_failed_txns;

} chronosServerStats_t;

/* Information required for update transactions */
typedef struct 
{
  const char *pkey;         /* Primary key */
} update_txn_info_t;

/* Information required for view_stock transactions */
typedef struct 
{
  const char *pkey;         /* Primary key */
} view_txn_info_t;

/* This is the structure of a transaction request in Chronos */
typedef struct 
{
  char txn_type;              /* Which transaction this is */
  chronos_time_t txn_start;
  chronos_time_t txn_enqueue;
  unsigned long long ticket;

  /* We access the following union based on the txn type */
  union {
    update_txn_info_t update_info;
    view_txn_info_t   view_info;
  } txn_specific_info;

} txn_info_t;
  
/* This is the structure of a ready queue in Chronos. 
 */
typedef struct 
{
  txn_info_t txnInfoArr[CHRONOS_READY_QUEUE_SIZE]; /* Txn requests are stored here */
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


typedef struct chronosServerContext_t 
{
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

  pthread_mutex_t admissionControlMutex;
  pthread_cond_t admissionControlWait;
  int txnToWait;
  
  /* Apart from client threads, we also have
   * update threads - threads that update the
   * data in the system periodically */
  int numUpdateThreads;

  /* This is the number of server threads that
   * dequeue transactions from the queue 
   * and process them */
  int numServerThreads;

  /* The duration of one chronos experiment */
  double duration_sec;
  
  /* Each data item is associated with a validity interval,
   * this is the initial value. */
  double validityIntervalms;

  /* Each data item is refreshed in a certain interval.
   * This update period is related to the validityIntervalms */
  double updatePeriodMS;
  int    numUpdatesPerUpdateThread;

  int serverPort;

  /* Whether an initial load is required or not */
  int initialLoad;

  int runningMode;
  int debugLevel;
  chronos_queue_t userTxnQueue;
  chronos_queue_t sysTxnQueue;

  /* These fields control the sampling task */
#define CHRONOS_SAMPLE_ARRAY_SIZE  (15*60*60)

  chronosServerStats_t  stats_matrix[CHRONOS_SAMPLING_SPACE][CHRONOS_MAX_NUM_SERVER_THREADS];
  double         average_service_delay_ms;
  double         degree_timing_violation;
  double         smoth_degree_timing_violation;
  double         alpha;

  long long AccessFrequency[CHRONOS_SAMPLE_ARRAY_SIZE][BENCHMARK_NUM_SYMBOLS];
  long long UpdateFrequency[CHRONOS_SAMPLE_ARRAY_SIZE][BENCHMARK_NUM_SYMBOLS];
  float AccessUpdateRatio[CHRONOS_SAMPLE_ARRAY_SIZE][BENCHMARK_NUM_SYMBOLS];
  long long FlexibleValidityInterval[CHRONOS_SAMPLE_ARRAY_SIZE][BENCHMARK_NUM_SYMBOLS];

  int numSamples;

  /* Metrics are obtained by sampling. This is the sampling interval */
  double samplingPeriodSec;

  volatile int currentSlot;
  struct timespec start;
  struct timespec end;

  /* Variables for the sampling timer */
  timer_t sampling_timer_id;
  struct itimerspec sampling_timer_et;
  struct sigevent sampling_timer_ev;

  /* Variables for the experiment timer */
  timer_t experiment_timer_id;
  struct itimerspec experiment_timer_et;
  struct sigevent experiment_timer_ev;

  int (*timeToDieFp)(void);

  /* These fields are for the benchmark framework */
  void *benchmarkCtxtP;
  
} chronosServerContext_t;

typedef struct chronosUpdateThreadInfo_t {
  int    num_stocks;       /* How many stocks should the thread handle */
  char **stocks_list;      /* Pointer to the array that contains the stocks managed by this thread */
} chronosUpdateThreadInfo_t;

typedef struct chronosServerThreadInfo_t {
  int       magic;
  pthread_t thread_id;
  int       thread_num;
  int       socket_fd;
  FILE     *trace_file;
  chronosServerThreadState_t state;
  chronosServerThreadType_t thread_type;
  chronosServerContext_t *contextP;

  /* These are fields specific to each thread type */
  union {
    chronosUpdateThreadInfo_t updateParameters;
  } parameters;
} chronosServerThreadInfo_t;

#endif
