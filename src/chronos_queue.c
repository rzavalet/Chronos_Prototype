#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "chronos_queue.h"
#include "chronos.h"
#include "chronos_transactions.h"

static int
chronos_dequeue_transaction(txn_info_t *txnInfoP, 
                            int (*timeToDieFp)(void), 
                            chronos_queue_t *txnQueueP)
{
  int              rc = CHRONOS_SUCCESS;
  struct timespec  ts; /* For the timed wait */

  if (txnQueueP == NULL || txnInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  pthread_mutex_lock(&txnQueueP->mutex);
  while(txnQueueP->occupied <= 0) {
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;
    pthread_cond_timedwait(&txnQueueP->more, &txnQueueP->mutex, &ts);

    if (timeToDieFp && timeToDieFp()) {
      chronos_info("Process asked to die");
      pthread_mutex_unlock(&txnQueueP->mutex);
      goto failXit;
    }
  }

  assert(txnQueueP->occupied > 0);

  memcpy(txnInfoP, &(txnQueueP->txnInfoArr[txnQueueP->nextout]), sizeof(*txnInfoP));

  chronos_info("Dequeued txn (%d)", txnQueueP->occupied);

  txnQueueP->nextout++;
  txnQueueP->nextout %= CHRONOS_READY_QUEUE_SIZE;
  txnQueueP->occupied--;
  /* now: either txnQueueP->occupied > 0 and txnQueueP->nextout is the index
       of the next occupied slot in the buffer, or
       txnQueueP->occupied == 0 and txnQueueP->nextout is the index of the next
       (empty) slot that will be filled by a producer (such as
       txnQueueP->nextout == txnQueueP->nextin) */

  pthread_cond_signal(&txnQueueP->less);
  pthread_mutex_unlock(&txnQueueP->mutex);
  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
chronos_enqueue_transaction(txn_info_t *txnInfoP, 
                            unsigned long long *ticket_ret, 
                            int (*timeToDieFp)(void), 
                            chronos_queue_t *txnQueueP)
{
  int              rc = CHRONOS_SUCCESS;
  struct timespec  ts; /* For the timed wait */

  if (txnQueueP == NULL || txnInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  pthread_mutex_lock(&txnQueueP->mutex);
   
  while (txnQueueP->occupied >= CHRONOS_READY_QUEUE_SIZE) {
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;

    pthread_cond_timedwait(&txnQueueP->less, &txnQueueP->mutex, &ts);

    if (timeToDieFp && timeToDieFp()) {
      chronos_info("Process asked to die");
      pthread_mutex_unlock(&txnQueueP->mutex);
      goto failXit;
    }
  }

  assert(txnQueueP->occupied < CHRONOS_READY_QUEUE_SIZE);
  
  chronos_info("Put a new update request in queue");

  memcpy(&(txnQueueP->txnInfoArr[txnQueueP->nextin]), txnInfoP, sizeof(*txnInfoP));

  txnQueueP->ticketReq ++;
  txnQueueP->txnInfoArr[txnQueueP->nextin].ticket = txnQueueP->ticketReq;
  if (ticket_ret) {
    *ticket_ret = txnQueueP->ticketReq;
  }
  
  txnQueueP->nextin ++;
  txnQueueP->nextin %= CHRONOS_READY_QUEUE_SIZE;
  txnQueueP->occupied++;

  chronos_info("Enqueued txn (%d), ticket: %lld",
               txnQueueP->occupied,
               txnQueueP->ticketReq);

  /* now: either b->occupied < CHRONOS_READY_QUEUE_SIZE and b->nextin is the index
       of the next empty slot in the buffer, or
       b->occupied == CHRONOS_READY_QUEUE_SIZE and b->nextin is the index of the
       next (occupied) slot that will be emptied by a dequeueTransaction
       (such as b->nextin == b->nextout) */

  pthread_cond_signal(&txnQueueP->more);

  pthread_mutex_unlock(&txnQueueP->mutex);

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

int
chronos_dequeue_system_transaction(const char **pkey, chronos_time_t *ts, chronosServerContext_t *contextP) 
{
  int              rc = CHRONOS_SUCCESS;
  txn_info_t       txn_info;
  chronos_queue_t *systemTxnQueueP = NULL;

  if (contextP == NULL || pkey == NULL || ts == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  systemTxnQueueP = &(contextP->sysTxnQueue);

  rc = chronos_dequeue_transaction(&txn_info, contextP->timeToDieFp, systemTxnQueueP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not dequeue update transaction");
    goto failXit;
  }

  *pkey = txn_info.txn_specific_info.update_info.pkey;
  *ts = txn_info.txn_enqueue;

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

int
chronos_enqueue_system_transaction(const char *pkey, const chronos_time_t *ts, chronosServerContext_t *contextP) 
{
  int              rc = CHRONOS_SUCCESS;
  txn_info_t       txn_info;
  chronos_queue_t *systemTxnQueueP = NULL;

  if (contextP == NULL || pkey == NULL || ts == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  systemTxnQueueP = &(contextP->sysTxnQueue);

  /* Set the transaction information */
  memset(&txn_info, 0, sizeof(txn_info));
  txn_info.txn_type = CHRONOS_SYS_TXN_UPDATE_STOCK;
  txn_info.txn_specific_info.update_info.pkey = pkey;
  txn_info.txn_enqueue = *ts;

  rc = chronos_enqueue_transaction(&txn_info, NULL, contextP->timeToDieFp, systemTxnQueueP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not enqueue update transaction");
    goto failXit;
  }

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

int
chronos_dequeue_user_transaction(void                   *requestP_ret, 
                                 chronos_time_t         *ts, 
                                 unsigned long long     *ticket_ret,
                                 volatile int           **txn_done_ret,
                                 chronosServerContext_t *contextP) 
{
  int              rc = CHRONOS_SUCCESS;
  txn_info_t       txn_info;
  chronos_queue_t *userTxnQueueP = NULL;

  if (contextP == NULL || ts == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  userTxnQueueP = &(contextP->userTxnQueue);

  rc = chronos_dequeue_transaction(&txn_info, contextP->timeToDieFp, userTxnQueueP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not dequeue user transaction");
    goto failXit;
  }

  memcpy(requestP_ret, &txn_info.request, sizeof(txn_info.request));
  *ts = txn_info.txn_enqueue;
  *ticket_ret = txn_info.ticket;
  *txn_done_ret = txn_info.txn_done;

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

int
chronos_enqueue_user_transaction(void *requestP,
                                 const chronos_time_t *ts, 
                                 unsigned long long *ticket_ret, 
                                 volatile int *txn_done,
                                 chronosServerContext_t *contextP) 
{
  int              rc = CHRONOS_SUCCESS;
  txn_info_t       txn_info;
  chronos_queue_t *userTxnQueueP = NULL;

  if (contextP == NULL || ts == NULL || ticket_ret == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  userTxnQueueP = &(contextP->userTxnQueue);

  /* Set the transaction information */
  memset(&txn_info, 0, sizeof(txn_info));
  memcpy(&txn_info.request, requestP, sizeof(txn_info.request));
  txn_info.txn_enqueue = *ts;
  txn_info.txn_done = txn_done;

  rc = chronos_enqueue_transaction(&txn_info, ticket_ret, contextP->timeToDieFp, userTxnQueueP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not enqueue update transaction");
    goto failXit;
  }

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}



