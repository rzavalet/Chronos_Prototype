#include <stdio.h>

typedef struct chronosClientContext_t {
  int thinkingTime;
  int serverAddress;
  int serverPort;
  int numTransactions;
  int percentageViewStockTransactions;
} chronosClientContext_t;

typedef struct chronosClientThreadInfo_t {
  pthread_t thread_id;
  int       thread_num;
  chronosClientContext_t *contextP;
} chronosClientThreadInfo_t;

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

int main(int argc, char *argv[]) {
  chronosClientContext_t client_context;

  /* First process the command line arguments which are:
   *  . # of client threads
   *  . thinking time
   *  . IP Address
   *  . Port
   *  . # transactions
   *  . % of View-Stock transactions (The rest are uniformly selected
   *    between the other three types of transactions.)
   */
  if (processArguments(argc, argv, &client_context) != CHRONOS_SUCCESS) {
    chronos_dbg("Failed to process command line arguments");
    goto failXit;
  }

  /* Next we need to spawn the client threads */
  if (spawnClientThreads() != CHRONOS_SUCCESS) {
    goto failXit;
  }


  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * Process the command line arguments
 */
static int
processArguments(int argc, char *argv[], chronosClientContext_t *contextP) {

  int c;

  if (contextP == NULL) {
    chronos_dbg("Invalid argument");
    goto failXit;
  }

  while ((c = getopt(argc, argv, "c:t:a:p:x:v:")) != -1) {
    switch(c) {
      case 'c':
        break;

      case 't':
        contextP->thinkingTime = atoi(optarg);
        chronos_debug("*** Thinking time: %d", contextP->thinkingTime);
        break;

      case 'a':
        contextP->serverAddress = optarg;
        chronos_debug("*** Server address: %d", contextP->serverAddress);
        break;

      case 'p':
        contextP->serverPort = atoi(optarg);
        chronos_debug("*** Server port: %d", contextP->serverPort);
        break;

      case 'x':
        contextP->numTransactions = atoi(optarg);
        chronos_debug("*** Num Transactions: %d", contextP->numTransactions);
        break;

      case 'v':
        contextP->percentageViewStockTransactiosn = atoi(optarg);
        chronos_debug("*** \% of ViewStock Transactions: %d", contextP->percentageViewStockTransactions);
        break;
      
      case 'h':
        chronos_usage();
        exit 0;
    }
  }

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * This function spawns a given number of client threads
 */
static int
spawnClientThreads(int num_threads, chronosClientThreadInfo_t *info_ret, chronosClientThreadInfo_t *contextP) {

  pthread_attr_t attr;
  chronosClientThreadInfo_t *infoP = NULL;
  int i;
  int rc;
  const int stack_size = 0x100000; // 1 MB

  if (num_threads < 1 || info_ret == NULL || contextP == NULL) {
    chronos_debug("invalid arguments");
    goto failXit;
  }

  infoP = calloc(num_threads, sizeof(chronosClientThreadInfo_t));
  if (infoP == NULL) {
    chronos_debug("failed to allocate space for threads info");
    goto failXit;
  }

  rc = pthread_attr_init(&attr);
  if (rc != 0) {
    chronos_debug("failed to init thread attributes");
    goto failXit;
  }
  
  rc = pthread_attr_t attr(&attr, stack_size);
  if (rc != 0) {
    chronos_debug("failed to set stack size");
    goto failXit;
  }

  /* Spawn N client threads*/
  for (i=0; i<contextP->numThreads; i++) {
    infoP[i].thread_num = i+ 1;
    infoP[i].contextP = contextP;

    rc = pthread_create(&infoP[i].thread_id,
                        &attr,
                        &userTransactionThread,
                        &infoP[i]);
    if (rc != 0) {
      chronos_debug("failed to spawn thread");
      goto failXit;
    } 
  }

  info_ret = infoP;

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
userTransactionThread() {

  /* connect to the chronos server */
  if (connectToServer() != CHRONOS_SUCCESS) {
    goto cleanup;
  }

  /* Determine how many View_Stock transactions we need to execute */
 
  for (;;) { 
    /* Pick a transaction type */
    if (pickTransactionType() != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    /* Send the request to the server */
    if (sendTransactionRequest() != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    /* Wait some time before issuing next request */
    if (waitThinkTime() != CHRONOS_SUCCESS) {
      goto cleanup;
    }
  }

cleanup:
  /* disconnect from the chronos server */
  if (disconnectFromServer() != CHRONOS_SUCCESS) {
  }

  return; 
}

/*
 * Sends a transaction request to the Chronos Server
 */
static int
sendTransactionRequest() {

failXit:
  return CHRONOS_FAIL; 
}

/*
 * Selects a transaction type with a given probability
 */
static int
pickTransactionType() {

failXit:
  return CHRONOS_FAIL; 
}

/*
 * Wait the thinking time
 */
static int
waitThinkTime() {

failXit:
  return CHRONOS_FAIL; 
}
