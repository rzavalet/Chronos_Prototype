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


int main(int argc, char *argv[]) 
{
  int rc;
  pthread_attr_t attr;
  int *thread_rc = NULL;
  const int stack_size = 0x100000; // 1 MB
  chronosServerContext_t server_context;
  chronosServerThreadInfo_t listenerThreadInfo;

  set_chronos_debug_level(CHRONOS_DEBUG_LEVEL_MIN+5);
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

  /* Create the system tables */
  if (benchmark_initial_load(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != CHRONOS_SUCCESS) {
    goto failXit;
  }

  if (benchmark_load_portfolio(CHRONOS_SERVER_HOME_DIR) != CHRONOS_SUCCESS) {
    goto failXit;
  }

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
  /* Initial population of system tables */
  if (populateChronosTables () != CHRONOS_SUCCESS) {
    goto failXit;
  }
#endif

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
#ifdef CHRONOS_NOT_YET_IMPLEMENTED

  /* Spawn processing thread */
  
  /* Spawn performance monitor thread */

  /* Spawn the update threads */
  for (;;) {
  }

  /* Spawn the client threads */
  for (;;) {
  }
#endif

  /* Spawn daListener thread */
  memset(&listenerThreadInfo, 0, sizeof(listenerThreadInfo));
  listenerThreadInfo.thread_type = CHRONOS_SERVER_THREAD_LISTENER;
  listenerThreadInfo.contextP = &server_context;

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
  
  while ((c = getopt(argc, argv, "c:v:u:t:p:h")) != -1) {
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

#ifdef CHRONOS_NOT_YET_IMPLEMENTED
/*
 * create the chronos tables
 */
static int
createChronosTables() {

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}


/*
 * populate the chronos tables
 */
static int
populateChronosTables() {

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

#endif

static int
dispatchTableFn (chronosRequestPacket_t *reqPacketP, chronosServerThreadInfo_t *infoP)
{
  chronosResponsePacket_t resPacket;
  int written, to_write;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  if (reqPacketP->txn_type == CHRONOS_USER_TXN_MAX) {
    infoP->state = CHRONOS_SERVER_THREAD_STATE_DIE;
  }
#ifdef CHRONOS_NOT_YET_IMPLEMENTED
  /* Do admission control */
  if (doAdmissionControl () != CHRONOS_SUCCESS) {
    goto failXit;
  }

  /* dispatch a transaction */
  switch(transactionType) {

    case viewStockTxn:
      return doViewStockTxn(); 
      break;

    case viewPortfolioTxn:
      return doViewPortfilioTxn();
      break;

    case purchaseTxn:
      return doPurchaseTxn();
      break;

    case saleTxn:
      return doSaleTxn();
      break

    default:
      assert(0);
  }
#endif
  memset(&resPacket, 0, sizeof(resPacket));
  resPacket.txn_type = reqPacketP->txn_type;
  resPacket.rc = CHRONOS_SUCCESS;

  to_write = sizeof(resPacket);
 
  while(to_write >0) {
    written = write(infoP->socket_fd, (char*)(&resPacket), to_write);
    if (written < 0) {
      chronos_error("Failed to write to socket");
      goto failXit;
    }

    to_write -= written;
  }

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

    if (infoP->state == CHRONOS_SERVER_THREAD_STATE_DIE) {
      chronos_info("Requested to die");
      goto cleanup;
    }
  }

cleanup:
  close(infoP->socket_fd);
  free(infoP);
  pthread_exit(NULL);
}

/*
 * listen for client requests
 */
static void *
daListener(void *argP) 
{
  int rc;
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
  }
cleanup:
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

/*
 * This is the driver function of an update thread
 */
static void
updateThread() {

  while (1) {
    /* Do adaptive update control */
    if (doAdaptiveUpdate () != CHRONOS_SUCCESS) {
      goto failXit;
    }

    if (updatePrices() != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    if (waitPeriod() != CHRONOS_SUCCESS) {
      goto cleanup;
    }
  }

cleanup:
  return;
}

/*
 * This is the driver function for performance monitoring
 */
static void
perfMonitorThread() {

  while (1) {
    if (computePerformanceError() != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    if (computeWorkloadAdjustment() != CHRONOS_SUCCESS) {
      goto cleanup;
    }
  }

cleanup:
  return;
}

/*
 * handler for view stock transaction
 */
static int
doViewStockTxn() {
 
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}


/*
 * handler for view portfolio transaction
 */
static int
doViewPortfolioTxn() {
 
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * handler for purchase transaction
 */
static int
doPurchaseTxn() {
 
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * handler for sale transaction
 */
static int
doSaleTxn() {
 
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * Wait till the next release time
 */
static int
waitPeriod() {

failXit:
  return CHRONOS_FAIL; 
}
#endif

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
