#include "startup_client.h"


/* This is the starting point of a Chronos Client.
 *
 * A client process spawns a number of threads as per
 * the user request. 
 *
 * According to the original paper, "a client thread first
 * needs to send the server a TCP connection request. It 
 * suspends until the server accepts the connection request
 * and allocates a server thread. When the connection is 
 * established, the client thread sends a transaction or 
 * query processing request and suspends until the 
 * corresponding server thread finishes processing the transaction
 * and returns the result. After receiving the result, the
 * client thread waits for a think time unit uniformly selected before
 * issuing another request."
 */

int main(int argc, char *argv[]) 
{
  chronosClientContext_t client_context;
  chronosClientThreadInfo_t *thread_infoP = NULL;
  int num_threads = 1;

  srand(time(NULL));

  set_chronos_debug_level(CHRONOS_DEBUG_LEVEL_MIN);
  memset(&client_context, 0, sizeof(client_context));

  /* First process the command line arguments which are:
   *  . # of client threads
   *  . thinking time
   *  . IP Address
   *  . Port
   *  . # transactions
   *  . % of View-Stock transactions (The rest are uniformly selected
   *    between the other three types of transactions.)
   */
  if (processArguments(argc, argv, &num_threads, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed to process command line arguments");
    goto failXit;
  }

  set_chronos_debug_level(client_context.debugLevel);
  
  /* Next we need to spawn the client threads */
  if (spawnClientThreads(num_threads, &thread_infoP, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed spawning threads");
    goto failXit;
  }

  if (thread_infoP == NULL) {
    chronos_error("Invalid thread info");
    goto failXit;
  }

  if (waitClientThreads(num_threads, thread_infoP, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed while waiting for threads termination");
    goto failXit;
  }

  printClientStats(num_threads, thread_infoP, &client_context);
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * Process the command line arguments
 */
static int
processArguments(int argc, char *argv[], int *num_threads, chronosClientContext_t *contextP) 
{

  int c;

  if (contextP == NULL || num_threads == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  if (argc < 2) {
    chronos_error("Invalid number of arguments");
    goto failXit;
  }

  memset(contextP, 0, sizeof(*contextP));
  
  while ((c = getopt(argc, argv, "n:c:t:a:p:x:v:d:h")) != -1) {
    switch(c) {
      case 'c':
        *num_threads = atoi(optarg);
        chronos_debug(2,"*** Num clients: %d", *num_threads);
        break;
      
      case 't':
        contextP->thinkingTime = atoi(optarg);
        chronos_debug(2, "*** Thinking time: %d", contextP->thinkingTime);
        break;

      case 'a':
        snprintf(contextP->serverAddress, sizeof(contextP->serverAddress), "%s", optarg);
        chronos_debug(2, "*** Server address: %s", contextP->serverAddress);
        break;

      case 'p':
        contextP->serverPort = atoi(optarg);
        chronos_debug(2, "*** Server port: %d", contextP->serverPort);
        break;

      case 'x':
        contextP->numTransactions = atoi(optarg);
        chronos_debug(2, "*** Num Transactions: %d", contextP->numTransactions);
        break;

      case 'v':
        contextP->percentageViewStockTransactions = atoi(optarg);
        chronos_debug(2, "*** %% of ViewStock Transactions: %d", contextP->percentageViewStockTransactions);
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

  if (*num_threads < 1) {
    chronos_error("number of clients must be > 0");
    goto failXit;
  }

  if (contextP->numTransactions <=0) {
    chronos_error("number of transactions must be > 0");
    goto failXit;
  }

  if (contextP->percentageViewStockTransactions <=0 || contextP->percentageViewStockTransactions > 100) {
    chronos_error("percentage of view_stock transactions must be between 1 and 100");
    goto failXit;
  }

  if (contextP->serverPort <= 0) {
    chronos_error("port must be a valid one");
    goto failXit;
  }

  if (contextP->serverAddress[0] == '\0') {
    chronos_error("address must be a valid one");
    goto failXit;
  }

  if (contextP->thinkingTime < 0) {
    chronos_error("Thinking time cannot be less than 0");
    goto failXit;
  }
  return CHRONOS_SUCCESS;

failXit:
  chronos_usage();
  return CHRONOS_FAIL;
}

/*
 * This function waits till client threads join
 */
static int
waitClientThreads(int num_threads, chronosClientThreadInfo_t *infoP, chronosClientContext_t *contextP) 
{
  int i;
  int rc;
  int *thread_rc = NULL;

  chronos_debug(5, "Waiting for client threads termination");
  for (i=0; i<num_threads; i++) {
    rc = pthread_join(infoP[i].thread_id, (void **)&thread_rc);
    if (rc != CHRONOS_SUCCESS) {
      chronos_error("Failed while joining thread %d", infoP[i].thread_num);
    }
    chronos_debug(4,"Thread %d finished", infoP[i].thread_num);
  }
  chronos_debug(5,"Done with client threads termination");

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * This function spawns a given number of client threads
 */
static int
spawnClientThreads(int num_threads, chronosClientThreadInfo_t **info_ret, chronosClientContext_t *contextP) 
{

  pthread_attr_t attr;
  chronosClientThreadInfo_t *infoP = NULL;
  int i;
  int rc;
  const int stack_size = 0x100000; // 1 MB

  if (num_threads < 1 || info_ret == NULL || contextP == NULL) {
    chronos_error("invalid arguments");
    goto failXit;
  }

  infoP = calloc(num_threads, sizeof(chronosClientThreadInfo_t));
  if (infoP == NULL) {
    chronos_error("failed to allocate space for threads info");
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

  /* Spawn N client threads*/
  chronos_debug(5,"Spawning %d client threads", num_threads);
  for (i=0; i<num_threads; i++) {
    infoP[i].thread_num = i+ 1;
    infoP[i].contextP = contextP;

    rc = pthread_create(&infoP[i].thread_id,
                        &attr,
                        &userTransactionThread,
                        &infoP[i]);
    if (rc != 0) {
      chronos_error("failed to spawn thread");
      goto failXit;
    }

    chronos_debug(4,"Spawed thread: %d", infoP[i].thread_num);
  }
  chronos_debug(5,"Done spawning %d client threads", num_threads);

  *info_ret = infoP;

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/* 
 * This is the callback function of a client thread. 
 * It receives the following information:
 *
 *  . thinking time
 *  . IP Address
 *  . Port
 *  . # transactions
 *  . % of View-Stock transactions (The rest are uniformly selected
 *    between the other three types of transactions.)
 *  
 *
 */
static void *
userTransactionThread(void *argP) 
{

  chronosClientThreadInfo_t *infoP = (chronosClientThreadInfo_t *)argP;
  chronos_user_transaction_t txnType;
  int i;
  int num_transactions;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  chronos_debug(3,"This is thread: %d", infoP->thread_num);
  num_transactions = infoP->contextP->numTransactions;

  /* connect to the chronos server */
  if (connectToServer(infoP) != CHRONOS_SUCCESS) {
    chronos_error("Could not connect to chronos server");
    goto cleanup;
  }

  /* Determine how many View_Stock transactions we need to execute */
  for (i=0; i<num_transactions; i++) { 
    /* Pick a transaction type */
    if (pickTransactionType(&txnType, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to pick transaction type");
      goto cleanup;
    }

    /* Send the request to the server */
    if (sendTransactionRequest(txnType, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to send transaction request");
      goto cleanup;
    }

    if (waitTransactionResponse(txnType, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to receive transaction response");
      goto cleanup;
    }

    /* Wait some time before issuing next request */
    if (waitThinkTime(infoP->contextP->thinkingTime) != CHRONOS_SUCCESS) {
      chronos_error("Error while waiting");
      goto cleanup;
    }
  }

  /* Send close request to the server */
  txnType = CHRONOS_USER_TXN_MAX;
  if (sendTransactionRequest(txnType, infoP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to send transaction request");
    goto cleanup;
  }

  if (waitTransactionResponse(txnType, infoP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to receive transaction response");
    goto cleanup;
  }

cleanup:
  /* disconnect from the chronos server */
  if (disconnectFromServer(infoP) != CHRONOS_SUCCESS) {
    chronos_error("Failed to disconnect from server");
  }
  pthread_exit(NULL);
}

/* 
 * Waits for response from chronos server
 */
static int
waitTransactionResponse(chronos_user_transaction_t txnType, chronosClientThreadInfo_t *infoP) 
{
  chronosResponsePacket_t resPacket;
  int num_bytes;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  memset(&resPacket, 0, sizeof(resPacket));

  num_bytes = read(infoP->socket_fd, &resPacket, sizeof(resPacket));

  if (num_bytes < 0) {
    chronos_error("Failed while reading response from server");
    goto failXit;
  }
 
  if (num_bytes != sizeof(resPacket)) {
    assert("Didn't receive enough bytes" == 0);
  }

  if (resPacket.txn_type == CHRONOS_USER_TXN_MAX) {
    chronos_info("Server closed socket for this client thread");
  }
  else {
    assert(resPacket.txn_type == txnType);
    chronos_debug(1,"Txn: %s, rc: %d", CHRONOS_TXN_NAME(resPacket.txn_type), resPacket.rc);
  }
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

/*
 * Sends a transaction request to the Chronos Server
 */
static int
sendTransactionRequest(chronos_user_transaction_t txnType, chronosClientThreadInfo_t *infoP) 
{
  chronosRequestPacket_t reqPacket;
  int written, to_write;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  chronos_debug(2, "Sending new transaction request: %s", CHRONOS_TXN_NAME(txnType));
  /* Populate a new transaction request */
  memset(&reqPacket, 0, sizeof(reqPacket));
  reqPacket.txn_type = txnType;

  to_write = sizeof(reqPacket);
 
  while(to_write >0) {
    written = write(infoP->socket_fd, (char*)(&reqPacket), to_write);
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
 * Selects a transaction type with a given probability
 */
static int
pickTransactionType(chronos_user_transaction_t *txn_type_ret, chronosClientThreadInfo_t *infoP) 
{
  int random_num;
  int percentage;;

  if (infoP == NULL || infoP->contextP == NULL || txn_type_ret == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  percentage = infoP->contextP->percentageViewStockTransactions;

  random_num = rand() % 100;

  if (random_num < percentage) {
    *txn_type_ret = CHRONOS_USER_TXN_VIEW_STOCK;
  }
  else {
    random_num = rand() % (CHRONOS_USER_TXN_MAX - 1);
    *txn_type_ret = CHRONOS_USER_TXN_INVAL;
    switch (random_num) {
      case 0:
        *txn_type_ret = CHRONOS_USER_TXN_VIEW_PORTFOLIO;
        break;

      case 1:
        *txn_type_ret = CHRONOS_USER_TXN_PURCHASE;
        break;

      case 2:
        *txn_type_ret = CHRONOS_USER_TXN_SALE;
        break;

      default:
        chronos_error("Transaction type: %d --> %d %s", random_num, *txn_type_ret, CHRONOS_TXN_NAME(*txn_type_ret));
        assert("Invalid transaction type" == 0);
        break;
    }
  }

  infoP->stats.txnCount[*txn_type_ret] ++;
  chronos_debug(3,"Selected transaction type is: %s", CHRONOS_TXN_NAME(*txn_type_ret));

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

static void
printClientStats(int num_threads, chronosClientThreadInfo_t *infoP, chronosClientContext_t *contextP) {
  int i;
#define num_columns  5

  const char title[] = "THREADS STATS";
  const char filler[] = "====================================================================================================";
  const char *column_names[num_columns] = {
                              "Thread Num",
                              "view_stock",
                              "view_portfolio",
                              "purchases",
                              "sales"};

  int len_title;
  int pad_size;
  const int  width = 80;
  int spaces = 0;

  len_title = strlen(title);
  pad_size = width / 2;

  spaces = (width) / (num_columns + 1);

  printf("%.*s\n", width, filler);
  printf("%*s\n", pad_size, title);
  printf("%.*s\n", width, filler);

  printf("%*s %*s %*s %*s %*s\n", 
          spaces,
          column_names[0], 
          spaces,
          column_names[1], 
          spaces,
          column_names[2], 
          spaces,
          column_names[3], 
          spaces,
          column_names[4]); 
  printf("%.*s\n", width, filler);

  for (i=0; i<num_threads; i++) {
    printf("%*d %*d %*d %*d %*d\n", 
            spaces,
            infoP[i].thread_num,
            spaces,
            infoP[i].stats.txnCount[CHRONOS_USER_TXN_VIEW_STOCK],
            spaces,
            infoP[i].stats.txnCount[CHRONOS_USER_TXN_VIEW_PORTFOLIO],
            spaces,
            infoP[i].stats.txnCount[CHRONOS_USER_TXN_PURCHASE],
            spaces,
            infoP[i].stats.txnCount[CHRONOS_USER_TXN_SALE]
    ); 
  }
}

static int
disconnectFromServer(chronosClientThreadInfo_t *infoP) 
{
  close(infoP->socket_fd);

  infoP->socket_fd = -1;
  return CHRONOS_SUCCESS; 
}

static int
connectToServer(chronosClientThreadInfo_t *infoP) 
{
  int socket_fd;
  int rc;

  struct sockaddr_in chronos_server_address;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    chronos_error("Could not open socket");
    goto failXit;
  }

  chronos_server_address.sin_family = AF_INET;
  chronos_server_address.sin_addr.s_addr = inet_addr(infoP->contextP->serverAddress);
  chronos_server_address.sin_port = htons(infoP->contextP->serverPort);
  
  rc = connect(socket_fd, (struct sockaddr *)&chronos_server_address, sizeof(chronos_server_address));
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not connect to server");
    goto failXit;
  }

  infoP->socket_fd = socket_fd;

  return CHRONOS_SUCCESS;

failXit:
  infoP->socket_fd = -1;
  return CHRONOS_FAIL; 
}

/*
 * Wait the thinking time
 */
static int
waitThinkTime(unsigned int thinkTime) 
{

  sleep(thinkTime);
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

static void
chronos_usage() 
{
  char usage[] =
    "Usage: startup_client OPTIONS\n"
    "Starts up a number of chronos clients\n"
    "\n"
    "OPTIONS:\n"
    "-c [num]              number of threads\n"
    "-t [num]              thinking time\n"
    "-a [address]          server ip address\n"
    "-p [num]              server port\n"
    "-x [num]              number of transactions\n"
    "-v [num]              percentage of user transactions\n"
    "-d [num]              debug level\n"
    "-h                    help";

  printf("%s\n", usage);
}
