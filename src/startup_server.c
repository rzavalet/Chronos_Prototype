#include "startup_server.h"

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
int timeToDie = 0;

int main(int argc, char *argv[]) 
{
  int rc;
  int i;
  int thread_num = 0;
  pthread_attr_t attr;
  int *thread_rc = NULL;
  const int stack_size = 0x100000; // 1 MB
  chronosServerContext_t server_context;
  chronosServerThreadInfo_t listenerThreadInfo;
  chronosServerThreadInfo_t *updateThreadInfoP = NULL;

  set_chronos_debug_level(CHRONOS_DEBUG_LEVEL_MIN);
  set_benchmark_debug_level(BENCHMARK_DEBUG_LEVEL_MIN);
  
  memset(&server_context, 0, sizeof(server_context));

  /* Process command line arguments which include:
   *
   *    - # of client threads it can accept.
   *    - validity interval of temporal data
   *    - # of update threads
   *    - update period
   *
   */
  if (processArguments(argc, argv, &server_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed to process arguments");
    goto failXit;
  }

  set_chronos_debug_level(server_context.debugLevel);
  set_benchmark_debug_level(server_context.debugLevel);
  
  /* Create the system tables */
  if (benchmark_initial_load(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != CHRONOS_SUCCESS) {
    chronos_error("Failed to perform initial load");
    goto failXit;
  }

  if (benchmark_load_portfolio(CHRONOS_SERVER_HOME_DIR) != CHRONOS_SUCCESS) {
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

  server_context.stats = calloc(server_context.numUpdateThreads, sizeof(chronosServerStats_t));
				
  /* Spawn processing thread */
  
  /* Spawn performance monitor thread */

  /* Spawn the update threads */
  updateThreadInfoP = calloc(server_context.numUpdateThreads, sizeof(*updateThreadInfoP));
  for (i=0; i<server_context.numUpdateThreads; i++) {
    updateThreadInfoP[i].thread_type = CHRONOS_SERVER_THREAD_UPDATE;
    updateThreadInfoP[i].contextP = &server_context;
    updateThreadInfoP[i].thread_num = thread_num ++;

    rc = pthread_create(&updateThreadInfoP[i].thread_id,
			&attr,
			&updateThread,
			updateThreadInfoP);
    if (rc != 0) {
      chronos_error("failed to spawn thread: %s", strerror(rc));
      goto failXit;
    }

    chronos_debug(4,"Spawed update thread: %d", updateThreadInfoP[i].thread_num);
  }

  /* Spawn daListener thread */
  memset(&listenerThreadInfo, 0, sizeof(listenerThreadInfo));
  listenerThreadInfo.thread_type = CHRONOS_SERVER_THREAD_LISTENER;
  listenerThreadInfo.contextP = &server_context;
  listenerThreadInfo.thread_num = thread_num ++;
    
  rc = pthread_create(&listenerThreadInfo.thread_id,
                      &attr,
                      &daListener,
                      &listenerThreadInfo);
  if (rc != 0) {
    chronos_error("failed to spawn thread: %s", strerror(rc));
    goto failXit;
  }

  chronos_debug(4,"Spawed listener thread");

  rc = pthread_join(listenerThreadInfo.thread_id, (void **)&thread_rc);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed while joining thread %s", listenerThreadInfo.thread_type);
  }

  for (i=0; i<server_context.numUpdateThreads; i++) {
    rc = pthread_join(updateThreadInfoP[i].thread_id, (void **)&thread_rc);
    if (rc != CHRONOS_SUCCESS) {
      chronos_error("Failed while joining thread %s", updateThreadInfoP[i].thread_type);
    }
  }
  
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * Process the command line arguments
 */
static int
processArguments(int argc, char *argv[], chronosServerContext_t *contextP) 
{
  int c;

  if (contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  if (argc < 2) {
    chronos_error("Invalid number of arguments");
    goto failXit;
  }

  memset(contextP, 0, sizeof(*contextP));
  
  while ((c = getopt(argc, argv, "c:v:u:t:p:d:h")) != -1) {
    switch(c) {
      case 'c':
        contextP->numClientsThreads = atoi(optarg);
        chronos_debug(2,"*** Num clients: %d", contextP->numClientsThreads);
        break;
      
      case 'v':
        contextP->validityInterval = atoi(optarg);
        chronos_debug(2, "*** Validity interval: %d", contextP->validityInterval);
        break;

      case 'u':
        contextP->numUpdateThreads = atoi(optarg);
        chronos_debug(2, "*** Num update threads: %d", contextP->numUpdateThreads);
        break;

      case 'p':
        contextP->serverPort = atoi(optarg);
        chronos_debug(2, "*** Server port: %d", contextP->serverPort);
        break;

      case 't':
        contextP->updatePeriod = atoi(optarg);
        chronos_debug(2, "*** Update period: %d", contextP->updatePeriod);
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

  if (contextP->validityInterval <= 0) {
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

  if (contextP->updatePeriod <= 0) {
    chronos_error("Update period must be > 0");
    goto failXit;
  }
  return CHRONOS_SUCCESS;

failXit:
  chronos_usage();
  return CHRONOS_FAIL;
}

static int
dispatchTableFn (chronosRequestPacket_t *reqPacketP, chronosServerThreadInfo_t *infoP)
{
  chronosResponsePacket_t resPacket;
  int written, to_write;
  int txn_rc = CHRONOS_SUCCESS;
  chronos_time_t begin_time;
  chronos_time_t end_time;
    
  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }
  
#ifdef CHRONOS_NOT_YET_IMPLEMENTED
  /* Do admission control */
  if (doAdmissionControl () != CHRONOS_SUCCESS) {
    goto failXit;
  }
#endif

  timerclear(&begin_time);
  timerclear(&end_time);
  
  if (getCurrentTime(&begin_time) != CHRONOS_SUCCESS) {
    chronos_error("Could not get begin time");
    goto failXit;
  }
  
  chronos_debug(3, "Processing transaction: %s", CHRONOS_TXN_NAME(reqPacketP->txn_type));
  /* dispatch a transaction */
  switch(reqPacketP->txn_type) {

  case CHRONOS_USER_TXN_VIEW_STOCK:
    txn_rc = benchmark_view_stock(CHRONOS_SERVER_HOME_DIR);
    break;

  case CHRONOS_USER_TXN_VIEW_PORTFOLIO:
    txn_rc = benchmark_view_portfolio(CHRONOS_SERVER_HOME_DIR);
    break;

  case CHRONOS_USER_TXN_PURCHASE:
    txn_rc = benchmark_purchase(CHRONOS_SERVER_HOME_DIR);
    break;

  case CHRONOS_USER_TXN_SALE:
    txn_rc = benchmark_sell(CHRONOS_SERVER_HOME_DIR);
    break;

  case CHRONOS_USER_TXN_MAX:
    infoP->state = CHRONOS_SERVER_THREAD_STATE_DIE;
    txn_rc = CHRONOS_SUCCESS;
    break;

  default:
    assert(0);
  }
  chronos_debug(3, "Done processing transaction: %s", CHRONOS_TXN_NAME(reqPacketP->txn_type));

  if (getCurrentTime(&end_time) != CHRONOS_SUCCESS) {
    chronos_error("Could not get end time");
    goto failXit;
  }
  
  if (reqPacketP->txn_type != CHRONOS_USER_TXN_MAX) {

    if (performanceMonitor(&begin_time, &end_time, reqPacketP->txn_type, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to execute performance monitor");
      goto failXit;
    }
  }
  
  chronos_debug(3, "Replying to client");
  
  memset(&resPacket, 0, sizeof(resPacket));
  resPacket.txn_type = reqPacketP->txn_type;
  resPacket.rc = txn_rc;

  to_write = sizeof(resPacket);
 
  while(to_write >0) {
    written = write(infoP->socket_fd, (char*)(&resPacket), to_write);
    if (written < 0) {
      chronos_error("Failed to write to socket");
      goto failXit;
    }

    to_write -= written;
  }
  chronos_debug(3, "Replied to client");
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
  chronosServerThreadInfo_t *infoP = (chronosServerThreadInfo_t *) argP;
  chronosRequestPacket_t reqPacket;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }
  
  while (1) {
    chronos_debug(5, "waiting new request");
    
    num_bytes = read(infoP->socket_fd, &reqPacket, sizeof(reqPacket));
    if (num_bytes < 0) {
      chronos_error("Failed while reading request from client");
      goto cleanup;
    }
   
    if (num_bytes != sizeof(reqPacket)) {
      assert("Didn't receive enough bytes" == 0);
    }

    assert(CHRONOS_TXN_IS_VALID(reqPacket.txn_type) || reqPacket.txn_type == CHRONOS_USER_TXN_MAX);
    chronos_debug(1, "Received transaction request: %s", CHRONOS_TXN_NAME(reqPacket.txn_type));

    if (dispatchTableFn(&reqPacket, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to handle request");
      goto cleanup;
    }

    if (timeToDie == 1) {
      infoP->state = CHRONOS_SERVER_THREAD_STATE_DIE;
    }
    
    if (infoP->state == CHRONOS_SERVER_THREAD_STATE_DIE) {
      chronos_info("Requested to die");
      goto cleanup;
    }
  }

cleanup:

  if (printStats(infoP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to print stats");
  }

  __sync_fetch_and_sub(&infoP->contextP->currentNumClients, 1);
  
  close(infoP->socket_fd);
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
  int num_clients = 0;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  int socket_fd;
  int accepted_socket_fd;
  int pid;
  int client_address_len;
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

  /* Keep listening for incoming connections */
  while(1) {

    chronos_debug(5, "Polling for new connection");
    
    /* wait for a new connection */
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

    rc = pthread_create(&handlerInfoP->thread_id,
                        &attr,
                        &daHandler,
                        handlerInfoP);
    if (rc != 0) {
      chronos_error("failed to spawn thread");
      goto cleanup;
    }

    chronos_debug(4,"Spawed handler thread");

    __sync_fetch_and_add(&infoP->contextP->currentNumClients, 1);

    if (timeToDie == 1) {
      infoP->state = CHRONOS_SERVER_THREAD_STATE_DIE;
    }
    
    if (infoP->state == CHRONOS_SERVER_THREAD_STATE_DIE) {
      chronos_info("Requested to die");
      goto cleanup;
    }
  }
  
cleanup:
  timeToDie = 1;
  chronos_info("daListener exiting");
  pthread_exit(NULL);
}

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
/*
 * This is the driver function of the main server thread
 */
static void
serverThread() {

  while (1) {
    if (waitPeriod() != CHRONOS_SUCCESS) {
      goto cleanup;
    }
  }

cleanup:
  return;
}
#endif

/*
 * This is the driver function of an update thread
 */
static void *
updateThread(void *argP) {
  
  chronosServerThreadInfo_t *infoP = (chronosServerThreadInfo_t *) argP;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  while (1) {
    chronos_debug(5, "new update period");
    
#ifdef CHRONOS_NOT_YET_IMPLEMENTED
    /* Do adaptive update control */
    if (doAdaptiveUpdate () != CHRONOS_SUCCESS) {
      goto cleanup;
    }
#endif

    if (benchmark_refresh_quotes(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != CHRONOS_SUCCESS) {
      chronos_error("Failed to refresh quotes");
      goto cleanup;
    }
    
    if (waitPeriod(infoP->contextP->updatePeriod) != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    if (timeToDie == 1) {
      infoP->state = CHRONOS_SERVER_THREAD_STATE_DIE;
    }
    
    if (infoP->state == CHRONOS_SERVER_THREAD_STATE_DIE) {
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

  assert(txn_type != CHRONOS_USER_TXN_MAX);
  timerclear(&elapsed_time);

  if (getElapsedTime(begin_time, end_time, &elapsed_time) != CHRONOS_SUCCESS) {
    chronos_error("Could not get elapsed time");
    goto failXit;
  }

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

/*
 * Wait till the next release time
 */
static int
waitPeriod(int updatePeriod)
{

  sleep(updatePeriod);
  
  return CHRONOS_SUCCESS; 
}

static int
getElapsedTime(chronos_time_t *begin, chronos_time_t *end, chronos_time_t *result)
{
  if (begin == NULL || end == NULL || result == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }
  
  timersub(end, begin, result);
  return CHRONOS_SUCCESS;

 failXit:
  return CHRONOS_FAIL;
}

static int
getCurrentTime(chronos_time_t *time)
{
  memset(time, 0, sizeof(*time));

  if (gettimeofday(time, NULL) != CHRONOS_SUCCESS) {
    chronos_error("Failed to retrieve current time");
    goto failXit;
  }
  
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
  
  thread_num = infoP->thread_num;
  stats = &infoP->contextP->stats[thread_num];
  
  timeradd(&stats->cumulative_time[txn_type], new_time, &result);

  stats->cumulative_time[txn_type] = result;
  stats->num_txns[txn_type] += 1;

  return CHRONOS_SUCCESS;
   
 failXit:
  return CHRONOS_FAIL;
}

static int
printStats(chronosServerThreadInfo_t *infoP)
{
  int i;
  chronosServerStats_t *stats;
  const char filler[] = "====================================================================================================";
  const int  width = 80;
  int thread_num;
  
  if (infoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  thread_num = infoP->thread_num;
  stats = &infoP->contextP->stats[thread_num];

  printf("%.*s\n", width, filler);
  for (i=0; i<CHRONOS_USER_TXN_MAX; i++) {

    CHRONOS_GET_AVERAGE(stats->cumulative_time[i], stats->num_txns[i], stats->average_time[i]);

    printf("Txn Type: %s\n", CHRONOS_TXN_NAME(i));
    printf("Num Transactions: %d\n", stats->num_txns[i]);
    printf("Total Execution time: "CHRONOS_TIME_FMT"\n", CHRONOS_TIME_ARG(stats->cumulative_time[i]));
    printf("Average Execution time: "CHRONOS_TIME_FMT"\n", CHRONOS_TIME_ARG(stats->average_time[i]));
    printf("\n");
  }
  printf("%.*s\n", width, filler);
  
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
    "-v [num]              validity interval\n"
    "-u [num]              number of update threads\n"
    "-t [num]              update period\n"
    "-p [num]              port to accept new connections\n"
    "-h                    help";

  printf("%s\n", usage);
}
