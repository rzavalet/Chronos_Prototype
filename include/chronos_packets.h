#ifndef _CHRONOS_PACKETS_H_
#define _CHRONOS_PACKETS_H_

#include "chronos_config.h"
#include "chronos_transactions.h"
#include "chronos_cache.h"
#include <stdlib.h>

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
