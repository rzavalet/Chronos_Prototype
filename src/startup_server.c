#include "startup_server.h"

/*===================== TODO LIST ==============================
 * 1) Destroy mutexes --- DONE
 * 2) Add overload detection
 * 3) Add Admision control
 * 4) Add Adaptive update
 * 5) Improve usage of Berkeley DB API
 *=============================================================*/
int time_to_die = 0;
chronosServerContext_t *serverContextP = NULL;
chronosServerThreadInfo_t *listenerThreadInfoP = NULL;
chronosServerThreadInfo_t *processingThreadInfoP = NULL;
chronosServerThreadInfo_t *updateThreadInfoArrP = NULL;
chronos_queue_t *userTxnQueueP = NULL;
chronos_queue_t *sysTxnQueueP = NULL;

void handler_sigint(int sig);

static int
printStats(chronosServerThreadInfo_t *infoP);

void handler_timer(void *arg);

/* This is the starting point for the Chronos Prototype. 
 * It has to perform the following tasks:
 *
 * 1) Create the tables in the system
 * 2) Populate the tables in the system
 * 3) Select the configuration of the workload.
 *    According to the original paper, these are the knobs of the system
 *    - # of client threads. In the original paper, they generated 300-900
 *                          client threads per machine -- they had 2 machines.
 *    - validity interval of temporal data
 *    - # of update threads
 *    - update period
 *    - thinking time for client threads
 *
 *    Additionally, in the original paper, there were several variables
 *    used in the experiments. For example:
 *
 *    - Running mode, which could be:
 *      . Base
 *      . Admission control
 *      . Adaptive updates
 *
 *  4) Spawn the update threads and start refreshing the data according to
 *     the update period. These threads will die when the run finishes
 *
 *     The validity interval in the original paper was 1s.
 *     The update period in the original paper was 0.5s
 *
 *  5) Spawn the client threads. Client threads will randomly pick a type of 
 *     workload:
 *      - 60% of client requests are View-Stock
 *      - 40% are uniformly selected among the other three types of user
 *        transactions.
 *
 *     The number of data accesses varies from 50 to 100.
 *     The think time in the paper is uniformly distributed in [0.3s, 0.5s]
 */ 
int main(int argc, char *argv[]) 
{
  int rc;
  int i;
  int thread_num = 0;
  pthread_attr_t attr;
  int *thread_rc = NULL;
  const int stack_size = 0x100000; // 1 MB

  set_chronos_debug_level(CHRONOS_DEBUG_LEVEL_MIN);
  set_benchmark_debug_level(BENCHMARK_DEBUG_LEVEL_MIN);

  serverContextP = malloc(sizeof(chronosServerContext_t));
  if (serverContextP == NULL) {
    chronos_error("Failed to allocate server context");
    goto failXit;
  }
  
  memset(serverContextP, 0, sizeof(chronosServerContext_t));

  /* Process command line arguments which include:
   *
   *    - # of client threads it can accept.
   *    - validity interval of temporal data
   *    - # of update threads
   *    - update period
   *
   */
  if (processArguments(argc, argv, serverContextP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to process arguments");
    goto failXit;
  }

  set_chronos_debug_level(serverContextP->debugLevel);
  set_benchmark_debug_level(serverContextP->debugLevel);
  
  /* set the signal hanlder for sigint */
  if (signal(SIGINT, handler_sigint) == SIG_ERR) {
    chronos_error("Failed to set signal handler");
    goto failXit;    
  }  

  /* Init the timer for sampling */
  serverContextP->timer_ev.sigev_notify = SIGEV_THREAD;
  serverContextP->timer_ev.sigev_value.sival_ptr = serverContextP;
  serverContextP->timer_ev.sigev_notify_function = (void *)handler_timer;
  serverContextP->timer_ev.sigev_notify_attributes = NULL;

  serverContextP->timer_et.it_interval.tv_sec = serverContextP->samplingPeriod;
  serverContextP->timer_et.it_interval.tv_nsec = 0;  
  serverContextP->timer_et.it_value.tv_sec = serverContextP->samplingPeriod;
  serverContextP->timer_et.it_value.tv_nsec = 0;

  chronos_info("Creating timer. Period: %ld", serverContextP->timer_et.it_interval.tv_sec);
  if (timer_create(CLOCK_REALTIME, &serverContextP->timer_ev, &serverContextP->timer_id) != 0) {
    chronos_error("Failed to create timer");
    goto failXit;
  }
  chronos_info("Done creating timer: %p", serverContextP->timer_id);

  userTxnQueueP = &(serverContextP->userTxnQueue);
  sysTxnQueueP = &(serverContextP->sysTxnQueue);

  serverContextP->magic = CHRONOS_SERVER_CTX_MAGIC;
  CHRONOS_SERVER_CTX_CHECK(serverContextP);
  
  if (pthread_mutex_init(&userTxnQueueP->mutex, NULL) != 0) {
    chronos_error("Failed to init mutex");
    goto failXit;
  }

  if (pthread_mutex_init(&sysTxnQueueP->mutex, NULL) != 0) {
    chronos_error("Failed to init mutex");
    goto failXit;
  }

  if (pthread_mutex_init(&serverContextP->startThreadsMutex, NULL) != 0) {
    chronos_error("Failed to init mutex");
    goto failXit;
  }

  if (pthread_cond_init(&sysTxnQueueP->more, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  if (pthread_cond_init(&sysTxnQueueP->less, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  if (pthread_cond_init(&sysTxnQueueP->ticketReady, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  if (pthread_cond_init(&userTxnQueueP->more, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  if (pthread_cond_init(&userTxnQueueP->less, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  if (pthread_cond_init(&userTxnQueueP->ticketReady, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  if (pthread_cond_init(&serverContextP->startThreadsWait, NULL) != 0) {
    chronos_error("Failed to init condition variable");
    goto failXit;
  }

  /* Create the system tables */
  if (benchmark_initial_load(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != CHRONOS_SUCCESS) {
    chronos_error("Failed to perform initial load");
    goto failXit;
  }

  /* Obtain a benchmark handle */
  if (benchmark_handle_alloc(&serverContextP->benchmarkCtxtP, CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != CHRONOS_SUCCESS) {
    chronos_error("Failed to allocate handle");
    goto failXit;
  }
  
  if (benchmark_load_portfolio(serverContextP->benchmarkCtxtP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to load portfolios");
    goto failXit;
  }

  rc = pthread_attr_init(&attr);
  if (rc != 0) {
    chronos_error("failed to init thread attributes");
    goto failXit;
  }
  
  rc = pthread_attr_setstacksize(&attr, stack_size);
  if (rc != 0) {
    chronos_error("failed to set stack size");
    goto failXit;
  }

  serverContextP->stats = calloc(serverContextP->numUpdateThreads, sizeof(chronosServerStats_t));
				
  /* Spawn processing thread */
  processingThreadInfoP = malloc(sizeof(chronosServerThreadInfo_t));
  if (processingThreadInfoP == NULL) {
    chronos_error("Failed to allocate thread structure");
    goto failXit;
  }

  memset(processingThreadInfoP, 0, sizeof(chronosServerThreadInfo_t));
  processingThreadInfoP->thread_type = CHRONOS_SERVER_THREAD_PROCESSING;
  processingThreadInfoP->contextP = serverContextP;
  processingThreadInfoP->thread_num = thread_num ++;
  processingThreadInfoP->magic = CHRONOS_SERVER_THREAD_MAGIC;
  
  rc = pthread_create(&processingThreadInfoP->thread_id,
                      &attr,
                      &processThread,
                      processingThreadInfoP);
  if (rc != 0) {
    chronos_error("failed to spawn thread: %s", strerror(rc));
    goto failXit;
  }

  chronos_debug(4,"Spawed processing thread");
  
  /* Spawn performance monitor thread */

  /* Spawn the update threads */
  updateThreadInfoArrP = calloc(serverContextP->numUpdateThreads, sizeof(chronosServerThreadInfo_t));
  if (updateThreadInfoArrP == NULL) {
    chronos_error("Failed to allocate thread structure");
    goto failXit;
  }

  for (i=0; i<serverContextP->numUpdateThreads; i++) {
    updateThreadInfoArrP[i].thread_type = CHRONOS_SERVER_THREAD_UPDATE;
    updateThreadInfoArrP[i].contextP = serverContextP;
    updateThreadInfoArrP[i].thread_num = thread_num ++;
    updateThreadInfoArrP[i].magic = CHRONOS_SERVER_THREAD_MAGIC;

    rc = pthread_create(&updateThreadInfoArrP[i].thread_id,
			&attr,
			&updateThread,
			&(updateThreadInfoArrP[i]));
    if (rc != 0) {
      chronos_error("failed to spawn thread: %s", strerror(rc));
      goto failXit;
    }

    chronos_debug(4,"Spawed update thread: %d", updateThreadInfoArrP[i].thread_num);
  }
#if 1
  /* Spawn daListener thread */
  listenerThreadInfoP = malloc(sizeof(chronosServerThreadInfo_t));
  if (listenerThreadInfoP == NULL) {
    chronos_error("Failed to allocate thread structure");
    goto failXit;
  }
  
  memset(listenerThreadInfoP, 0, sizeof(chronosServerThreadInfo_t));
  listenerThreadInfoP->thread_type = CHRONOS_SERVER_THREAD_LISTENER;
  listenerThreadInfoP->contextP = serverContextP;
  listenerThreadInfoP->thread_num = thread_num ++;
  listenerThreadInfoP->magic = CHRONOS_SERVER_THREAD_MAGIC;
    
  rc = pthread_create(&listenerThreadInfoP->thread_id,
                      &attr,
                      &daListener,
                      listenerThreadInfoP);
  if (rc != 0) {
    chronos_error("failed to spawn thread: %s", strerror(rc));
    goto failXit;
  }

  chronos_debug(4,"Spawed listener thread");

  rc = pthread_join(listenerThreadInfoP->thread_id, (void **)&thread_rc);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed while joining thread %s", CHRONOS_SERVER_THREAD_NAME(listenerThreadInfoP->thread_type));
  }
#endif
  for (i=0; i<serverContextP->numUpdateThreads; i++) {
    rc = pthread_join(updateThreadInfoArrP[i].thread_id, (void **)&thread_rc);
    if (rc != CHRONOS_SUCCESS) {
      chronos_error("Failed while joining thread %s", CHRONOS_SERVER_THREAD_NAME(updateThreadInfoArrP[i].thread_type));
    }
  }
  
  rc = pthread_join(processingThreadInfoP->thread_id, (void **)&thread_rc);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed while joining thread %s", CHRONOS_SERVER_THREAD_NAME(processingThreadInfoP->thread_type));
  }

  rc = CHRONOS_SUCCESS;
  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;
  
 cleanup:

  /* Obtain a benchmark handle */
  if (benchmark_handle_free(serverContextP->benchmarkCtxtP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to free handle");
    goto failXit;
  }
  
  
  pthread_mutex_destroy(&userTxnQueueP->mutex);
  pthread_mutex_destroy(&sysTxnQueueP->mutex);
  pthread_cond_destroy(&serverContextP->startThreadsWait);
  pthread_mutex_destroy(&serverContextP->startThreadsMutex);

  return rc;
}

void handler_sigint(int sig)
{
  printf("Received signal: %d\n", sig);
  time_to_die = 1;
}

void handler_timer(void *arg)
{
  chronosServerContext_t *contextP = (chronosServerContext_t *) arg;
  int currentSlot;
  
  chronos_info("****** TIMER... *****");
  CHRONOS_SERVER_CTX_CHECK(serverContextP);

  /*======= Obtain average of the last period ========*/
  currentSlot = contextP->currentSlot;

  CHRONOS_TIME_AVERAGE(contextP->txn_delay[currentSlot], contextP->txn_count[currentSlot], contextP->txn_avg_delay[currentSlot]);
  chronos_info("Acc: "CHRONOS_TIME_FMT", Num: %llu, AVG: "CHRONOS_TIME_FMT, CHRONOS_TIME_ARG(contextP->txn_delay[currentSlot]), contextP->txn_count[currentSlot], CHRONOS_TIME_ARG(contextP->txn_avg_delay[currentSlot]));

  contextP->txn_enqueued[currentSlot] = contextP->userTxnQueue.occupied;
  
  contextP->currentSlot = (contextP->currentSlot + 1) % CHRONOS_SAMPLE_ARRAY_SIZE;
  contextP->numSamples ++;
}

/*
 * Process the command line arguments
 */
static int
processArguments(int argc, char *argv[], chronosServerContext_t *contextP) 
{
  int c;
  int ms, ms2;

  if (contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  if (argc < 2) {
    chronos_error("Invalid number of arguments");
    goto failXit;
  }

  memset(contextP, 0, sizeof(*contextP));
  
  while ((c = getopt(argc, argv, "c:v:s:u:r:t:p:d:h")) != -1) {
    switch(c) {
      case 'c':
        contextP->numClientsThreads = atoi(optarg);
        chronos_debug(2,"*** Num clients: %d", contextP->numClientsThreads);
        break;
      
      case 'v':
	ms = atoi(optarg);
	contextP->validityInterval.tv_sec = ms / 1000;
	contextP->validityInterval.tv_nsec = (ms % 1000) * 1000000;
	chronos_debug(2, "*** Validity interval: "CHRONOS_TIME_FMT, CHRONOS_TIME_ARG(contextP->validityInterval));
        break;

      case 'u':
        contextP->numUpdateThreads = atoi(optarg);
        chronos_debug(2, "*** Num update threads: %d", contextP->numUpdateThreads);
        break;

      case 'p':
        contextP->serverPort = atoi(optarg);
        chronos_debug(2, "*** Server port: %d", contextP->serverPort);
        break;

      case 's':
        contextP->samplingPeriod = atoi(optarg);
        chronos_debug(2, "*** Sampling period: %d [secs]", contextP->samplingPeriod);
        break;

      case 't':
	ms2 = atoi(optarg);
	contextP->updatePeriod.tv_sec = ms2 / 1000;
	contextP->updatePeriod.tv_nsec = (ms2 % 1000) * 1000000;
	chronos_debug(2, "*** Update period: "CHRONOS_TIME_FMT, CHRONOS_TIME_ARG(contextP->updatePeriod));
        break;

      case 'r':
        contextP->duration = atoi(optarg);
        chronos_debug(2, "*** Duration: %d", contextP->duration);
        break;

      case 'd':
        contextP->debugLevel = atoi(optarg);
        chronos_debug(2, "*** Debug Level: %d", contextP->debugLevel);
        break;

      case 'h':
        chronos_usage();
        exit(0);
	      break;

      default:
        chronos_error("Invalid argument");
        goto failXit;
    }
  }

  if (contextP->numClientsThreads < 1) {
    chronos_error("number of clients must be > 0");
    goto failXit;
  }

  if (ms <= 0) {
    chronos_error("validity interval must be > 0");
    goto failXit;
  }

  if (contextP->numUpdateThreads <= 0) {
    chronos_error("number of update threads must be > 0");
    goto failXit;
  }

  if (contextP->serverPort <= 0) {
    chronos_error("port must be a valid one");
    goto failXit;
  }

  if (ms2 <= 0) {
    chronos_error("Update period must be > 0");
    goto failXit;
  }
  return CHRONOS_SUCCESS;

failXit:
  chronos_usage();
  return CHRONOS_FAIL;
}

static in
waitForTransaction(chronos_queue_t *b, unsigned long long ticket)
{
  int rc = CHRONOS_SUCCESS;
  
  pthread_mutex_lock(&b->mutex);
  while(ticket <= b->ticketDone)
    pthread_cond_wait(&b->ticketReady, &b->mutex);
  pthread_mutex_unlock(&b->mutex);

  return rc;
}

static unsigned long long
enqueueTransaction(chronos_queue_t *b)
{
  unsigned long long ticket;
  
  pthread_mutex_lock(&b->mutex);
  while (b->occupied >= BSIZE)
    pthread_cond_wait(&b->less, &b->mutex);

  assert(b->occupied < BSIZE);
  b->buf[b->nextin++] = reqPacketP->txn_type;
  b->nextin %= BSIZE;
  b->occupied++;
  b->ticketReq++;
  ticket = b->ticketReq;
  pthread_cond_signal(&b->more);
  pthread_mutex_unlock(&b->mutex);

  return ticket;
}

static chronos_user_transaction_t 
dequeueTransaction(chronos_queue_t *b)
{
  chronos_user_transaction_t item;
  pthread_mutex_lock(&b->mutex);
  while(b->occupied <= 0)
    pthread_cond_wait(&b->more, &b->mutex);

  assert(b->occupied > 0);

  item = b->buf[b->nextout++];
  b->nextout %= BSIZE;
  b->occupied--;
  /* now: either b->occupied > 0 and b->nextout is the index
       of the next occupied slot in the buffer, or
       b->occupied == 0 and b->nextout is the index of the next
       (empty) slot that will be filled by a producer (such as
       b->nextout == b->nextin) */

  pthread_cond_signal(&b->less);
  pthread_mutex_unlock(&b->mutex);

  return(item);
}

static int
dispatchTableFn (chronosRequestPacket_t *reqPacketP, chronosServerThreadInfo_t *infoP)
{
  int current_slot = 0;
  unsigned long long ticket;
  chronos_time_t begin_time;
  chronos_time_t end_time;
  chronos_queue_t *userTxnQueueP = NULL;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  CHRONOS_SERVER_THREAD_CHECK(infoP);
  CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

  userTxnQueueP = &(infoP->contextP->userTxnQueue);

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
  /* Do admission control */
  if (doAdmissionControl () != CHRONOS_SUCCESS) {
    goto failXit;
  }
#endif

  CHRONOS_TIME_CLEAR(begin_time);
  CHRONOS_TIME_CLEAR(end_time);

  CHRONOS_TIME_GET(begin_time);

  /*===========================================
   * Put a new transaction in the txn queue
   *==========================================*/
  chronos_debug(3, "Processing transaction: %s", CHRONOS_TXN_NAME(reqPacketP->txn_type));
  
  current_slot = infoP->contextP->currentSlot;  
  infoP->contextP->txn_received[current_slot] += 1;

  ticket = enqueueTransaction(userTxnQueueP, reqPacketP->txn_type);

  /* Wait till the transaction is done */
  waitForTransaction(userTxnQueueP, ticket);
  
  chronos_debug(3, "Done processing transaction: %s", CHRONOS_TXN_NAME(reqPacketP->txn_type));


  CHRONOS_TIME_GET(end_time);

#if 0
  /*===========================================
   * Run the performance monitor
   * NOTE: This should happen from time to time
   *==========================================*/
  if (performanceMonitor(&begin_time, &end_time, reqPacketP->txn_type, infoP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to execute performance monitor");
    goto failXit;
  }
#endif
  
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * handle a transaction request
 */
static void *
daHandler(void *argP) 
{
  int num_bytes;
  chronosResponsePacket_t resPacket;
  int written, to_write;
  chronosServerThreadInfo_t *infoP = (chronosServerThreadInfo_t *) argP;
  chronosRequestPacket_t reqPacket;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  CHRONOS_SERVER_THREAD_CHECK(infoP);
  CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

  /*=======================================================
   * Wait here till all threads are initialized 
   *======================================================*/
  pthread_mutex_lock(&infoP->contextP->startThreadsMutex);
  
  while (infoP->contextP->currentNumClients < infoP->contextP->numClientsThreads && time_to_die == 0) {
    chronos_debug(4,"Waiting for client threads to startup");
    pthread_cond_wait(&infoP->contextP->startThreadsWait, &infoP->contextP->startThreadsMutex);
  }
  
  pthread_mutex_unlock(&infoP->contextP->startThreadsMutex);


  /*=====================================================
   * Keep receiving new transaction requests until it 
   * is time to die.
   *====================================================*/
  while (1) {

    CHRONOS_SERVER_THREAD_CHECK(infoP);
    CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

    /*------------ Read the request -----------------*/
    chronos_debug(5, "waiting new request");
    
    num_bytes = read(infoP->socket_fd, &reqPacket, sizeof(reqPacket));
    if (num_bytes < 0) {
      chronos_error("Failed while reading request from client");
      goto cleanup;
    }
   
    if (num_bytes != sizeof(reqPacket)) {
      chronos_info("Client disconnected");
      goto cleanup;
    }

    assert(CHRONOS_TXN_IS_VALID(reqPacket.txn_type));
    chronos_debug(1, "Received transaction request: %s", CHRONOS_TXN_NAME(reqPacket.txn_type));
    /*-----------------------------------------------*/

    
    /*----------- Process the request ----------------*/
    if (dispatchTableFn(&reqPacket, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to handle request");
      goto cleanup;
    }
    /*-----------------------------------------------*/

    
    /*---------- Reply to the request ---------------*/
    chronos_debug(3, "Replying to client");
  
    memset(&resPacket, 0, sizeof(resPacket));
    resPacket.txn_type = reqPacket.txn_type;
    resPacket.rc = CHRONOS_SUCCESS;

    to_write = sizeof(resPacket);
 
    while(to_write >0) {
      written = write(infoP->socket_fd, (char*)(&resPacket), to_write);
      if (written < 0) {
	chronos_error("Failed to write to socket");
	goto cleanup;
      }

      to_write -= written;
    }
    chronos_debug(3, "Replied to client");
    /*-----------------------------------------------*/

    
    if (time_to_die == 1) {
      chronos_info("Requested to die");
      goto cleanup;
    }
  }

cleanup:

  close(infoP->socket_fd);

  pthread_mutex_lock(&infoP->contextP->startThreadsMutex);
  infoP->contextP->currentNumClients -= 1;
  if (infoP->contextP->currentNumClients == 0) {
    time_to_die = 1;
  }
  pthread_mutex_unlock(&infoP->contextP->startThreadsMutex);

  free(infoP);

  chronos_info("daHandler exiting");
  pthread_exit(NULL);
}

/*
 * listen for client requests
 */
static void *
daListener(void *argP) 
{
  int rc;
  int done_creating = 0;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  int socket_fd;
  int accepted_socket_fd;

  socklen_t client_address_len;
  pthread_attr_t attr;
  const int stack_size = 0x100000; // 1 MB
  chronosServerThreadInfo_t *handlerInfoP = NULL;

  chronosServerThreadInfo_t *infoP = (chronosServerThreadInfo_t *) argP;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  rc = pthread_attr_init(&attr);
  if (rc != 0) {
    chronos_error("failed to init thread attributes");
    goto cleanup;
  }
  
  rc = pthread_attr_setstacksize(&attr, stack_size);
  if (rc != 0) {
    chronos_error("failed to set stack size");
    goto cleanup;
  }

  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (rc != 0) {
    chronos_error("failed to set detachable attribute");
    goto cleanup;
  }

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    chronos_error("Failed to create socket");
    goto cleanup;
  }

  server_address.sin_family = AF_INET;
  // Perhaps I need to change the ip address in the following line
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port = htons(infoP->contextP->serverPort);

  rc = bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed to bind socket");
    goto cleanup;
  }

  rc = listen(socket_fd, infoP->contextP->numClientsThreads);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed to set listen queue");
    goto cleanup;
  }

  chronos_debug(2, "Waiting for incoming connections...");

  /* Keep listening for incoming connections till we
   * complete all threads for the experiment
   */
  while(!done_creating && !time_to_die) {

    chronos_debug(5, "Polling for new connection");
    
    /* wait for a new connection */
    /* TODO: make the call abort-able */
    client_address_len = sizeof(client_address); 
    accepted_socket_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_len);
    if (accepted_socket_fd == -1) {
      chronos_error("Failed to accept connection");
      goto cleanup;
    }

    handlerInfoP  = NULL;
    handlerInfoP = calloc(1, sizeof(chronosServerThreadInfo_t));
    if (handlerInfoP == NULL) {
      chronos_error("Failed to allocate space for thread info");
      goto cleanup;
    }

    // TODO: What would be needed to use a thread pool instead?
    handlerInfoP->socket_fd = accepted_socket_fd;
    handlerInfoP->contextP = infoP->contextP;
    handlerInfoP->state = CHRONOS_SERVER_THREAD_STATE_RUN;
    handlerInfoP->magic = CHRONOS_SERVER_THREAD_MAGIC;

    rc = pthread_create(&handlerInfoP->thread_id,
                        &attr,
                        &daHandler,
                        handlerInfoP);
    if (rc != 0) {
      chronos_error("failed to spawn thread");
      goto cleanup;
    }

    chronos_debug(4,"Spawed handler thread");

    /*===========================================================
     * Signal the threads for the fact that all of them have
     * been created and that they can start creating transactions
     *==========================================================*/
    pthread_mutex_lock(&infoP->contextP->startThreadsMutex);
    infoP->contextP->currentNumClients += 1;
    if (infoP->contextP->currentNumClients == infoP->contextP->numClientsThreads) {
      done_creating = 1;
      chronos_info("daListener created all requested threads");
    }
    pthread_cond_signal(&infoP->contextP->startThreadsWait);
    pthread_mutex_unlock(&infoP->contextP->startThreadsMutex);
  }
  
cleanup:
  chronos_info("daListener exiting");
  pthread_exit(NULL);
}

/*
 * Wait till the next release time
 */
static int
waitPeriod(chronos_time_t updatePeriod)
{
  /* TODO: do I need to check the second argument? */
  nanosleep(&updatePeriod, NULL);
  
  return CHRONOS_SUCCESS; 
}

/*
 * This is the driver function of process thread
 */
static void *
processThread(void *argP) 
{
  int txn_rc = CHRONOS_SUCCESS;
  chronosServerThreadInfo_t *infoP = (chronosServerThreadInfo_t *) argP;
  chronos_queue_t *userTxnQueueP = NULL;
  chronos_queue_t *sysTxnQueueP = NULL;
  chronos_user_transaction_t txn_type;
  chronos_time_t system_start, system_end, txn_start, txn_end;
  chronos_time_t txn_duration, txn_delay;
  int current_slot = 0;
  
  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  CHRONOS_SERVER_THREAD_CHECK(infoP);
  CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

  userTxnQueueP = &(infoP->contextP->userTxnQueue);
  sysTxnQueueP = &(infoP->contextP->sysTxnQueue);

  CHRONOS_TIME_GET(system_start);
  infoP->contextP->start = system_start;

  chronos_info("Starting timer %p...", infoP->contextP->timer_id);
  if (timer_settime(infoP->contextP->timer_id, 0, &infoP->contextP->timer_et, NULL) != 0) {
    chronos_error("Failed to start timer");
    goto cleanup;
  }

  /* Give update transactions more priority */
  /* TODO: Add scheduling technique */
  while (!time_to_die) {

    CHRONOS_SERVER_THREAD_CHECK(infoP);
    CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

    /*-------- Process refresh transaction ----------*/
    if (sysTxnQueueP->occupied > 0) {
      chronos_info("Processing update...");
      
      current_slot = infoP->contextP->currentSlot;
      txn_type = dequeueTransaction(sysTxnQueueP);
      if (benchmark_refresh_quotes(infoP->contextP->benchmarkCtxtP) != CHRONOS_SUCCESS) {
        chronos_error("Failed to refresh quotes");
        goto cleanup;
      }
      
      infoP->contextP->txn_update[current_slot] += 1;      
      chronos_info("Done processing update...");
    }
    /*--------------------------------------------*/

    CHRONOS_SERVER_THREAD_CHECK(infoP);
    CHRONOS_SERVER_CTX_CHECK(infoP->contextP);
    
    /*-------- Process user transaction ----------*/
    if (userTxnQueueP->occupied > 0) {
      chronos_info("Processing user txn...");

      current_slot = infoP->contextP->currentSlot;

      CHRONOS_TIME_GET(txn_start);
      txn_type = dequeueTransaction(userTxnQueueP);
      
        /* dispatch a transaction */
      switch(txn_type) {

      case CHRONOS_USER_TXN_VIEW_STOCK:
        txn_rc = benchmark_view_stock(infoP->contextP->benchmarkCtxtP);
        break;

      case CHRONOS_USER_TXN_VIEW_PORTFOLIO:
        txn_rc = benchmark_view_portfolio(infoP->contextP->benchmarkCtxtP);
        break;

      case CHRONOS_USER_TXN_PURCHASE:
        txn_rc = benchmark_purchase(infoP->contextP->benchmarkCtxtP);
        break;

      case CHRONOS_USER_TXN_SALE:
        txn_rc = benchmark_sell(infoP->contextP->benchmarkCtxtP);
        break;

      default:
        assert(0);
      }

      /* Notify waiter thread that this txn is done */
      pthread_mutex_lock(&userTxnQueueP->mutex);
      userTxnQueueP->ticketDone ++;
      pthread_cond_signal(&userTxnQueueP->ticketReady);
      pthread_mutex_unlock(&userTxnQueueP->mutex);
      /*--------------------------------------------*/


      if (txn_rc != CHRONOS_SUCCESS) {
	chronos_error("Transaction failed");
      }
      
      CHRONOS_TIME_GET(txn_end);
      /* One more transasction finished */
      infoP->contextP->txn_count[current_slot] += 1;

      /* Calculate the delay of this transaction */
      CHRONOS_TIME_NANO_OFFSET_GET(txn_start, txn_end, txn_duration);
      chronos_info("txn start: "CHRONOS_TIME_FMT", txn end: "CHRONOS_TIME_FMT", duration: "CHRONOS_TIME_FMT,
		   CHRONOS_TIME_ARG(txn_start), CHRONOS_TIME_ARG(txn_end), CHRONOS_TIME_ARG(txn_duration));
      
      CHRONOS_TIME_NANO_OFFSET_GET(infoP->contextP->validityInterval, txn_duration, txn_delay);
      chronos_info("txn duration: "CHRONOS_TIME_FMT", validity interval: "CHRONOS_TIME_FMT", delay: "CHRONOS_TIME_FMT,
		   CHRONOS_TIME_ARG(txn_duration), CHRONOS_TIME_ARG(infoP->contextP->validityInterval), CHRONOS_TIME_ARG(txn_delay));
      
      CHRONOS_TIME_ADD(infoP->contextP->txn_delay[current_slot], txn_delay, infoP->contextP->txn_delay[current_slot]);
      chronos_info("txn delay acc: "CHRONOS_TIME_FMT,
		   CHRONOS_TIME_ARG(infoP->contextP->txn_delay[current_slot]));
      
      chronos_info("Done processing user txn...");
    }

  }
  
cleanup:
  CHRONOS_TIME_GET(system_end);
  infoP->contextP->end = system_end;
  
  printStats(infoP);

  chronos_info("processThread exiting");
  
  pthread_exit(NULL);
}

/*
 * This is the driver function of an update thread
 */
static void *
updateThread(void *argP) {
  
  chronosServerThreadInfo_t *infoP = (chronosServerThreadInfo_t *) argP;
  chronos_queue_t *userTxnQueueP = NULL;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  CHRONOS_SERVER_THREAD_CHECK(infoP);
  CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

  userTxnQueueP = &(infoP->contextP->sysTxnQueue);

  while (1) {
    CHRONOS_SERVER_THREAD_CHECK(infoP);
    CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

    chronos_debug(5, "new update period");
   
#ifdef CHRONOS_NOT_YET_IMPLEMENTED
    /* Do adaptive update control */
    if (doAdaptiveUpdate () != CHRONOS_SUCCESS) {
      goto cleanup;
    }
#endif

    pthread_mutex_lock(&userTxnQueueP->mutex);
     
    while (userTxnQueueP->occupied >= BSIZE)
      pthread_cond_wait(&userTxnQueueP->less, &userTxnQueueP->mutex);

    assert(userTxnQueueP->occupied < BSIZE);
    
    chronos_info("Put a new update request in queue");
    userTxnQueueP->buf[userTxnQueueP->nextin++] = CHRONOS_SYS_TXN_UPDATE_STOCK;
    
    userTxnQueueP->nextin %= BSIZE;
    userTxnQueueP->occupied++;

    /* now: either b->occupied < BSIZE and b->nextin is the index
         of the next empty slot in the buffer, or
         b->occupied == BSIZE and b->nextin is the index of the
         next (occupied) slot that will be emptied by a dequeueTransaction
         (such as b->nextin == b->nextout) */

    pthread_cond_signal(&userTxnQueueP->more);

    pthread_mutex_unlock(&userTxnQueueP->mutex);

    if (waitPeriod(infoP->contextP->updatePeriod) != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    if (time_to_die == 1) {
      chronos_info("Requested to die");
      goto cleanup;
    }
  }

cleanup:
  chronos_info("updateThread exiting");
  pthread_exit(NULL);
}

/*
 * This is the driver function for performance monitoring
 */
static int
performanceMonitor(chronos_time_t *begin_time, chronos_time_t *end_time, chronos_user_transaction_t txn_type, chronosServerThreadInfo_t *infoP)
{
  chronos_time_t elapsed_time;

  if (begin_time == NULL || end_time == NULL || infoP == NULL) {
    chronos_error("Invalid Arguments");
    goto failXit;
  }
  
  CHRONOS_SERVER_THREAD_CHECK(infoP);
  CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

  assert(txn_type != CHRONOS_USER_TXN_MAX);
  CHRONOS_TIME_CLEAR(elapsed_time);

  CHRONOS_TIME_NANO_OFFSET_GET(*begin_time, *end_time, elapsed_time);

  chronos_debug(3, "Start Time: "CHRONOS_TIME_FMT": End Time: "CHRONOS_TIME_FMT" Elapsed Time: "CHRONOS_TIME_FMT, CHRONOS_TIME_ARG(*begin_time), CHRONOS_TIME_ARG(*end_time), CHRONOS_TIME_ARG(elapsed_time));
  
  if ( updateStats(txn_type, &elapsed_time, infoP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to update stats");
    goto failXit;
  }

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
  if (computePerformanceError() != CHRONOS_SUCCESS) {
    goto cleanup;
  }

  if (computeWorkloadAdjustment() != CHRONOS_SUCCESS) {
    goto cleanup;
  }
#endif
  
  return CHRONOS_SUCCESS;

 failXit:
  return CHRONOS_FAIL;
}

static int
updateStats(chronos_user_transaction_t txn_type, chronos_time_t *new_time, chronosServerThreadInfo_t *infoP)
{
  chronos_time_t result;
  chronosServerStats_t *stats;
  int thread_num;
  
  assert(txn_type != CHRONOS_USER_TXN_MAX);
  
  if (new_time == NULL || infoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  CHRONOS_SERVER_THREAD_CHECK(infoP);
  CHRONOS_SERVER_CTX_CHECK(infoP->contextP);

  thread_num = infoP->thread_num;
  stats = &infoP->contextP->stats[thread_num];

  CHRONOS_TIME_ADD(stats->cumulative_time[txn_type], *new_time, result);

  stats->cumulative_time[txn_type] = result;
  stats->num_txns[txn_type] += 1;

  return CHRONOS_SUCCESS;
   
 failXit:
  return CHRONOS_FAIL;
}

static int
printStats(chronosServerThreadInfo_t *infoP)
{
#define num_columns  6
  int i;
  const char title[] = "TRANSACTIONS STATS";
  const char filler[] = "========================================================================================================================";
  const char *column_names[num_columns] = {
                              "Sample",
                              "Xacts Received",
			      "Xacts Enqueued",
			      "Processed Xacts",
                              "Average Delay",
                              "Update Xacts"};

  int pad_size;
  int  width;
  int spaces = 0;

  if (infoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  width = strlen(filler);
  pad_size = width / 2;
  spaces = (width) / (num_columns + 1);

  printf("%.*s\n", width, filler);
  printf("%*s\n", pad_size, title);
  printf("%.*s\n", width, filler);

  printf("%*s %*s %*s %*s %*s %*s\n",
	 spaces,
	 column_names[0], 
	 spaces,
	 column_names[1],
	 spaces,
	 column_names[2], 
	 spaces,
	 column_names[3],
	 spaces,
	 column_names[4],
	 spaces,
	 column_names[5]);
  printf("%.*s\n", width, filler);

  for (i=0; i<infoP->contextP->numSamples; i++) {
    printf("%*d %*llu %*llu %*llu %*s"CHRONOS_TIME_FMT" %*llu\n",
	   spaces,
	   i,
	   spaces,
	   infoP->contextP->txn_received[i],
	   spaces,
	   infoP->contextP->txn_enqueued[i],
	   spaces,
	   infoP->contextP->txn_count[i],
	   spaces > 9 ? spaces - 9 : spaces,
	   "",
	   CHRONOS_TIME_ARG(infoP->contextP->txn_avg_delay[i]),
	   spaces,
	   infoP->contextP->txn_update[i]);
  }
  
  return CHRONOS_SUCCESS;
  
 failXit:
  return CHRONOS_FAIL;
}

static void
chronos_usage() 
{
  char usage[] =
    "Usage: startup_server OPTIONS\n"
    "Starts up a chronos server \n"
    "\n"
    "OPTIONS:\n"
    "-c [num]              number of clients it can accept\n"
    "-v [num]              validity interval [in milliseconds]\n"
    "-u [num]              number of update threads\n"
    "-r [num]              duration of the experiment [in minutes]\n"
    "-t [num]              update period [in milliseconds]\n"
    "-s [num]              sampling period [in seconds]\n"
    "-p [num]              port to accept new connections\n"
    "-d [num]              debug level\n"
    "-h                    help";

  printf("%s\n", usage);
}
