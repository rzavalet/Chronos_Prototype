#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "chronos_queue.h"
#include "chronos.h"

static int
chronos_dequeue_transaction(txn_info_t *txnInfoP, chronos_queue_t *txnQueueP)
{
  int              rc = CHRONOS_SUCCESS;

  if (txnQueueP == NULL || txnInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  pthread_mutex_lock(&txnQueueP->mutex);
  while(txnQueueP->occupied <= 0)
    pthread_cond_wait(&txnQueueP->more, &txnQueueP->mutex);

  assert(txnQueueP->occupied > 0);

  memcpy(txnInfoP, &(txnQueueP->txnInfoArr[txnQueueP->nextout]), sizeof(*txnInfoP));

  chronos_info("Dequeued txn (%d), type: %d",
               txnQueueP->occupied,
               txnInfoP->txn_type);

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
chronos_enqueue_transaction(txn_info_t *txnInfoP, chronos_queue_t *txnQueueP)
{
  int              rc = CHRONOS_SUCCESS;

  if (txnQueueP == NULL || txnInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  pthread_mutex_lock(&txnQueueP->mutex);
   
  while (txnQueueP->occupied >= CHRONOS_READY_QUEUE_SIZE)
    pthread_cond_wait(&txnQueueP->less, &txnQueueP->mutex);

  assert(txnQueueP->occupied < CHRONOS_READY_QUEUE_SIZE);
  
  chronos_info("Put a new update request in queue");

  memcpy(&(txnQueueP->txnInfoArr[txnQueueP->nextin++]), txnInfoP, sizeof(*txnInfoP));
  
  txnQueueP->nextin %= CHRONOS_READY_QUEUE_SIZE;
  txnQueueP->occupied++;

  chronos_info("Enqueued txn (%d), type: %d",
               txnQueueP->occupied,
               txnInfoP->txn_type);

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
chronos_dequeue_system_transaction(const char **pkey, chronosServerContext_t *contextP) 
{
  int              rc = CHRONOS_SUCCESS;
  txn_info_t       txn_info;
  chronos_queue_t *systemTxnQueueP = NULL;

  if (contextP == NULL || pkey == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  systemTxnQueueP = &(contextP->sysTxnQueue);

  rc = chronos_dequeue_transaction(&txn_info, systemTxnQueueP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not dequeue update transaction");
    goto failXit;
  }

  assert(txn_info.txn_type == CHRONOS_USER_TXN_VIEW_STOCK);
  *pkey = txn_info.txn_specific_info.update_info.pkey;

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

int
chronos_enqueue_system_transaction(const char *pkey, chronosServerContext_t *contextP) 
{
  int              rc = CHRONOS_SUCCESS;
  txn_info_t       txn_info;
  chronos_queue_t *systemTxnQueueP = NULL;

  if (contextP == NULL || pkey == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  systemTxnQueueP = &(contextP->sysTxnQueue);

  /* Set the transaction information */
  memset(&txn_info, 0, sizeof(txn_info));
  txn_info.txn_type = CHRONOS_SYS_TXN_UPDATE_STOCK;
  txn_info.txn_specific_info.update_info.pkey = pkey;

  rc = chronos_enqueue_transaction(&txn_info, systemTxnQueueP);
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


