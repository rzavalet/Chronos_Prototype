#ifndef _CHRONOS_PACKETS_H_
#define _CHRONOS_PACKETS_H_

#include "chronos_config.h"
#include "chronos_transactions.h"
#include "chronos_cache.h"
#include <stdlib.h>

typedef struct chronosResponsePacket_t {
  chronosUserTransaction_t txn_type;
  int rc;
} chronosResponsePacket_t;

typedef struct chronosRequestPacket_t {
  chronosUserTransaction_t txn_type;

  /* A transaction can affect up to 100 symbols */
  int numItems;
  union {
    chronosViewPortfolioInfo_t portfolioInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
    chronosSymbol_t            symbolInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
    chronosPurchaseInfo_t      purchaseInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
    chronosSellInfo_t          sellInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
  } request_data;

} chronosRequestPacket_t;

typedef void *chronosRequest;
typedef void *chronosResponse;

chronosRequest
chronosRequestCreate(chronosUserTransaction_t txnType, 
                     chronosCache chronosCacheH);

int
chronosRequestFree(chronosRequest requestH);

chronosUserTransaction_t
chronosRequestTypeGet(chronosRequest requestH);

size_t
chronosRequestSizeGet(chronosRequest requestH);

chronosResponse
chronosResponseAlloc();

int
chronosResponseFree(chronosResponse responseH);

size_t
chronosResponseSizeGet(chronosRequest requestH);

chronosUserTransaction_t
chronosResponseTypeGet(chronosResponse responseH);

int
chronosResponseResultGet(chronosResponse responseH);
#endif
