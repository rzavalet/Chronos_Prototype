#include "chronos_config.h"
#include "chronos_client.h"
#include "chronos_packets.h"
#include "startup_client.h"
#include "chronos_transaction_names.h"
#include "benchmark_common.h"
#include <signal.h>

int time_to_die = 0;

void handler_sigint(int sig);

#if 0
static void
printClientStats(int num_threads, chronosClientThreadInfo_t *infoP, chronosClientContext_t *contextP);
#endif

int isTimeToDie()
{
  return time_to_die;
}

/*
 * Wait the thinking time
 */
static int
waitThinkTime(int minThinkTimeMS, int maxThinkTimeMS) 
{
  struct timespec waitPeriod;
  int randomWaitTimeMS;

  randomWaitTimeMS = minThinkTimeMS + (rand() % (1 + maxThinkTimeMS - minThinkTimeMS));
   
  waitPeriod.tv_sec = randomWaitTimeMS / 1000;
  waitPeriod.tv_nsec = ((int)randomWaitTimeMS % 1000) * 1000000;

  /* TODO: do I need to check the second argument? */
  chronos_info("Waiting for %d ms", randomWaitTimeMS);
  nanosleep(&waitPeriod, NULL);
  
  return CHRONOS_SUCCESS; 
}

#define xstr(a) str(a)
#define str(a) #a

static void
chronos_usage() 
{
  char usage[] =
    "Usage: startup_client OPTIONS\n"
    "Starts up a number of chronos clients\n"
    "\n"
    "OPTIONS:\n"
    "-c [num]              number of threads (default: "xstr(CHRONOS_NUM_CLIENT_THREADS)")\n"
    "-a [address]          server ip address (default: "CHRONOS_SERVER_ADDRESS")\n"
    "-p [num]              server port (default: "xstr(CHRONOS_SERVER_PORT)")\n"
    "-v [num]              percentage of user transactions (default: "xstr(CHRONOS_RATE_VIEW_TRANSACTIONS)"%)\n"
    "-d [num]              debug level\n"
    "-h                    help";

  printf("%s\n", usage);
}

#define CHRONOS_CLIENT_NUM_STOCKS     (300)
#define CHRONOS_CLIENT_NUM_USERS      (50)

static int
populateRequest(chronos_user_transaction_t txnType, chronosRequestPacket_t *reqPacketP, chronosClientThreadInfo_t *infoP)
{
  int rc = CHRONOS_SUCCESS;
  int i;
  int random_symbol;
  int random_num_symbols = CHRONOS_MIN_DATA_ITEMS_PER_XACT + rand() % (1 + CHRONOS_MAX_DATA_ITEMS_PER_XACT - CHRONOS_MIN_DATA_ITEMS_PER_XACT);
  char **listP = NULL;

  if (reqPacketP == NULL || infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  listP = infoP->contextP->stocksListP;

  memset(reqPacketP, 0, sizeof(*reqPacketP));
  reqPacketP->txn_type = txnType;
  reqPacketP->numSymbols = random_num_symbols;

  switch (txnType) {
    case CHRONOS_USER_TXN_VIEW_STOCK:
      for (i=0; i<random_num_symbols; i++) {
        random_symbol = rand() % CHRONOS_CLIENT_NUM_STOCKS;
        chronosPackViewStock(random_symbol, 
                             listP[random_symbol], 
                             &(reqPacketP->request_data.symbolInfo[i]));
      }

      break;

    case CHRONOS_USER_TXN_VIEW_PORTFOLIO:
      for (i=0; i<random_num_symbols; i++) {
        char accountId[ID_SZ];

        random_symbol = rand() % CHRONOS_CLIENT_NUM_USERS;
        snprintf(accountId, sizeof(accountId),
                 "%d", random_symbol);
        chronosPackViewPortfolio(accountId,
                                 &(reqPacketP->request_data.portfolioInfo[i]));
      }
      break;

    case CHRONOS_USER_TXN_PURCHASE:
      break;

    case CHRONOS_USER_TXN_SALE:
      break;

    default:
      assert("Invalid transaction type" == 0);
  }

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
stockListFree(chronosClientContext_t *contextP) 
{
  int rc = CHRONOS_SUCCESS;
  int i;

  if (contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  if (contextP->stocksListP != NULL) {
    for (i=0; i<CHRONOS_CLIENT_NUM_STOCKS; i++) {
      if (contextP->stocksListP[i] != NULL) {
        free(contextP->stocksListP[i]);
        contextP->stocksListP[i] = NULL;
      }
    }

    free(contextP->stocksListP);
    contextP->stocksListP = NULL;
  }

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
stockListFromFile(char *homedir, char *datafilesdir, chronosClientContext_t *contextP)
{
  int rc = CHRONOS_SUCCESS;
  int current_slot = 0;
  int size;
  char *stocks_file = NULL;
  char **listP = NULL;
  FILE *ifp;
  char buf[MAXLINE];
  char ignore_buf[500];
  STOCK my_stocks;

  assert(homedir != NULL && homedir[0] != '\0');
  assert(datafilesdir != NULL && datafilesdir[0] != '\0');
  
  if (contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  size = strlen(datafilesdir) + strlen(STOCKS_FILE) + 2;
  stocks_file = malloc(size);
  if (stocks_file == NULL) {
    benchmark_error("Failed to allocate memory.");
    goto failXit;
  }
  snprintf(stocks_file, size, "%s/%s", datafilesdir, STOCKS_FILE);

  listP = calloc(CHRONOS_CLIENT_NUM_STOCKS, sizeof (char *));
  if (listP == NULL) {
    benchmark_error("Could not allocate storage for stocks list");
    goto failXit;
  }

  ifp = fopen(stocks_file, "r");
  if (ifp == NULL) {
    benchmark_error("Error opening file '%s'", stocks_file);
    goto failXit;
  }

  /* Iterate over the vendor file */
  while (fgets(buf, MAXLINE, ifp) != NULL && current_slot < CHRONOS_CLIENT_NUM_STOCKS) {

    /* zero out the structure */
    memset(&my_stocks, 0, sizeof(STOCK));

    /*
     * Scan the line into the structure.
     * Convenient, but not particularly safe.
     * In a real program, there would be a lot more
     * defensive code here.
     */
    sscanf(buf,
      "%10[^#]#%128[^#]#%500[^\n]",
      my_stocks.stock_symbol, my_stocks.full_name, ignore_buf);

    listP[current_slot] = strdup(my_stocks.stock_symbol);
    current_slot ++;
  }

  fclose(ifp);
  goto cleanup;

failXit:
  if (ifp != NULL) {
    fclose(ifp);
  }

  if (listP != NULL) {
    int i;
    for (i=0; i<CHRONOS_CLIENT_NUM_STOCKS; i++) {
      if (listP[i] != NULL) {
        free(listP[i]);
        listP[i] = NULL;
      }
    }

    free(listP);
    listP = NULL;
  }

  rc = CHRONOS_FAIL;

cleanup:
  if (contextP != NULL) {
    contextP->stocksListP = listP;
  }

  return rc;
}

static int
initProcessArguments(chronosClientContext_t *contextP)
{
  strncpy(contextP->serverAddress, CHRONOS_SERVER_ADDRESS, sizeof(contextP->serverAddress));
  contextP->serverPort = CHRONOS_SERVER_PORT;
  contextP->percentageViewStockTransactions = CHRONOS_RATE_VIEW_TRANSACTIONS;
  contextP->numClientsThreads = CHRONOS_NUM_CLIENT_THREADS;
  contextP->minThinkingTime = CHRONOS_MIN_THINK_TIME_MS;
  contextP->maxThinkingTime = CHRONOS_MAX_THINK_TIME_MS;
  contextP->timeToDieFp = isTimeToDie;

#ifdef CHRONOS_DEBUG
  contextP->debugLevel = CHRONOS_DEBUG_LEVEL_MAX;
#endif

  return 0;
}
/*
 * Process the command line arguments
 */
static int
processArguments(int argc, char *argv[], chronosClientContext_t *contextP) 
{

  int c;

  if (contextP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  memset(contextP, 0, sizeof(*contextP));

  initProcessArguments(contextP);

  while ((c = getopt(argc, argv, "n:c:a:p:v:d:h")) != -1) {
    switch(c) {
      case 'c':
        contextP->numClientsThreads = atoi(optarg);
        chronos_debug(2,"*** Num clients: %d", contextP->numClientsThreads);
        break;

      case 'a':
        snprintf(contextP->serverAddress, sizeof(contextP->serverAddress), "%s", optarg);
        chronos_debug(2, "*** Server address: %s", contextP->serverAddress);
        break;

      case 'p':
        contextP->serverPort = atoi(optarg);
        chronos_debug(2, "*** Server port: %d", contextP->serverPort);
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

  if (contextP->numClientsThreads <= 0) {
    chronos_error("number of clients must be > 0");
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

void handler_sigint(int sig)
{
  printf("Received signal: %d\n", sig);
  time_to_die = 1;
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
  chronosConnHandle connectionH = NULL;
  chronos_user_transaction_t txnType;
  int cnt_txns = 0;
  int rc = CHRONOS_SUCCESS;

  if (infoP == NULL || infoP->contextP == NULL) {
    chronos_error("Invalid argument");
    goto cleanup;
  }

  chronos_debug(3,"This is thread: %d", infoP->thread_num);

  connectionH = chronosConnHandleAlloc();
  if (connectionH == NULL) {
    chronos_error("Could not allocate connection handle");
    goto cleanup;
  }

  /* connect to the chronos server */
  rc = chronosClientConnect(infoP->contextP->serverAddress,
                            infoP->contextP->serverPort,
                            NULL,
                            connectionH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not connect to chronos server");
    goto cleanup;
  }

  /* Determine how many View_Stock transactions we need to execute */
  while(1) {
    chronosRequestPacket_t reqPacket;
    
    /* Pick a transaction type */
    if (pickTransactionType(&txnType, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to pick transaction type");
      goto cleanup;
    }

    if (populateRequest(txnType, &reqPacket, infoP) != CHRONOS_SUCCESS) {
      chronos_error("Failed to populate request");
      goto cleanup;
    }

    /* Send the request to the server */
    rc = chronosClientSendRequest(&reqPacket, connectionH);
    if ( rc != CHRONOS_SUCCESS) {
      chronos_error("Failed to send transaction request");
      goto cleanup;
    }

    cnt_txns ++;
    chronos_debug(3,"[thr: %d] txn count: %d", infoP->thread_num, cnt_txns);
    
    rc = chronosClientReceiveResponse(connectionH);
    if (rc != CHRONOS_SUCCESS) {
      chronos_error("Failed to receive transaction response");
      goto cleanup;
    }

    /* Wait some time before issuing next request */
    if (waitThinkTime(infoP->contextP->minThinkingTime,
                      infoP->contextP->maxThinkingTime) != CHRONOS_SUCCESS) {
      chronos_error("Error while waiting");
      goto cleanup;
    }

    if (time_to_die == 1) {
      chronos_info("Requested termination");
      break;
    }
  }

cleanup:
  /* disconnect from the chronos server */
  rc = chronosClientDisconnect(connectionH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed to disconnect from server");
  }

  rc = chronosConnHandleFree(connectionH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed to free connection handle");
  }

  pthread_exit(NULL);
}

/*
 * Selects a transaction type with a given probability
 */
static int
pickTransactionType(chronos_user_transaction_t *txn_type_ret, chronosClientThreadInfo_t *infoP) 
{
#ifdef CHRONOS_ALL_TXN_AVAILABLE
  int random_num;
  int percentage;
#endif

  if (infoP == NULL || infoP->contextP == NULL || txn_type_ret == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

#ifdef CHRONOS_ALL_TXN_AVAILABLE
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
#else
  *txn_type_ret = CHRONOS_USER_TXN_VIEW_STOCK;
#endif

  infoP->stats.txnCount[*txn_type_ret] ++;
  chronos_debug(3,"Selected transaction type is: %s", CHRONOS_TXN_NAME(*txn_type_ret));

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

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

  srand(time(NULL));

  set_chronos_debug_level(CHRONOS_DEBUG_LEVEL_MIN+4);
  memset(&client_context, 0, sizeof(client_context));

  if (processArguments(argc, argv, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed to process command line arguments");
    goto failXit;
  }

  set_chronos_debug_level(client_context.debugLevel);

  /* set the signal hanlder for sigint */
  if (signal(SIGINT, handler_sigint) == SIG_ERR) {
    chronos_error("Failed to set signal handler");
    goto failXit;    
  }
  
  if (stockListFromFile(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed to retrieve stock symbols");
    goto failXit;    
  }

  /* Next we need to spawn the client threads */
  if (spawnClientThreads(client_context.numClientsThreads, &thread_infoP, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed spawning threads");
    goto failXit;
  }

  if (thread_infoP == NULL) {
    chronos_error("Invalid thread info");
    goto failXit;
  }

  if (waitClientThreads(client_context.numClientsThreads, thread_infoP, &client_context) != CHRONOS_SUCCESS) {
    chronos_error("Failed while waiting for threads termination");
    goto failXit;
  }

#if 0
  printClientStats(client_context.numClientsThreads, thread_infoP, &client_context);
#endif
  stockListFree(&client_context);

  return CHRONOS_SUCCESS;

failXit:
  stockListFree(&client_context);
  return CHRONOS_FAIL;
}

