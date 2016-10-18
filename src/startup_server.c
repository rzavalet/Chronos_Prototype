#include <stdio.h>

typedef struct chronosServerContext_t {
  int numClientsThreads;
  int numUpdateThreads;
  int validityInterval;
  int updatePeriod;
  int runningMode;
} chronosServerContext_t;

chronos_queue_t userTransactionsQueue;
chronos_queue_t updateTransactionsQueue;

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


int main() {

  /* Process command line arguments which include:
   *
   *    - # of client threads it can accept.
   *    - validity interval of temporal data
   *    - # of update threads
   *    - update period
   *    - thinking time for client threads
   *
   */
  if (processArguments() != CHRONOS_SUCCESS) {
    goto failXit;
  }

  /* Create the system tables */
  if (createChronosTables () != CHRONOS_SUCCESS) {
    goto failXit;
  }

  /* Initial population of system tables */
  if (populateChronosTables () != CHRONOS_SUCCESS) {
    goto failXit;
  }

  /* Spawn processing thread */
  
  /* Spawn performance monitor thread */

  /* Spawn the update threads */
  for (;;) {
  }

  /* Spawn the client threads */
  for (;;) {
  }

  /* Spawn daListener thread */

  /* Spawn daHandler thread*/


  return 0;
}

/*
 * Process the command line arguments
 */
static int
processArguments() {

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}


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

/*
 * receive a request from a client thread
 */
static int
receiveRequest() {

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}


/*
 * handle a transaction request
 */
static int
daHandler() {

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

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * listen for client requests
 */
static int
daListener() {

  /* setup listening port */
  if (setupListener () != CHRONOS_SUCCESS) {
    goto failXit;
  }

  /* Keep listening for incomming connections */
  while(1) {
    /* wait for a new connection */
    if (acceptConnectionFromClient () != CHRONOS_SUCCESS) {
      goto failXit;
    }

    /* give the new connection a handler thread */
    if (assignClientThread() != CHRONOS_SUCCESS) {
      goto failXit;
    }

  }
 
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}


/*
 * accept an incoming connection
 */
static int
acceptConnectionFromClient() {

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

/*
 * assign a client thread
 */
static int
assignClientThread() {


  /* Check if we can accept more clients */
  if (spareClientThreads() != CHRONOS_SUCCESS) {
    goto done;
  }

done:
  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}


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
 * This is the driver function of a client thread
 */
static void
clientThread() {

  while (1) {
    if (receiveRequest() != CHRONOS_SUCCESS) {
      goto cleanup;
    }

    /* Do admission control */
    if (doAdmissionControl () != CHRONOS_SUCCESS) {
      goto failXit;
    }

    if (daHandler() != CHRONOS_SUCCESS) {
      goto cleanup;
    }
  }

cleanup:
  return;
}

/*
 * setup the listening port
 */
static int
setupListener() {

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
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
