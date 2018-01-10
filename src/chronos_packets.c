#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "chronos.h"
#include "chronos_config.h"
#include "chronos_transactions.h"
#include "chronos_packets.h"
#include "chronos_environment.h"
#include "chronos_cache.h"

const char *chronos_user_transaction_str[] = {
  "CHRONOS_USER_TXN_VIEW_STOCK",
  "CHRONOS_USER_TXN_VIEW_PORTFOLIO",
  "CHRONOS_USER_TXN_PURCHASE",
  "CHRONOS_USER_TXN_SALE"
};

const char *chronos_system_transaction_str[] = {
  "CHRONOS_SYS_TXN_UPDATE_STOCK"
};

static int
chronosPackPurchase(const char *accountId,
                    int          symbolId, 
                    const char *symbol, 
                    float        price,
                    int          amount,
                    chronosPurchaseInfo_t *purchaseInfoP)
{
  int rc = CHRONOS_SUCCESS;

  if (purchaseInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  strncpy(purchaseInfoP->accountId, 
          accountId,
          sizeof(purchaseInfoP->accountId));

  purchaseInfoP->symbolId = symbolId;
  strncpy(purchaseInfoP->symbol, 
          symbol,
          sizeof(purchaseInfoP->symbol));

  purchaseInfoP->price = price;
  purchaseInfoP->amount = amount;

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
chronosPackSellStock(const char *accountId,
                     int          symbolId, 
                     const char *symbol, 
                     float        price,
                     int          amount,
                     chronosSellInfo_t *sellInfoP)
{
  int rc = CHRONOS_SUCCESS;

  if (sellInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  strncpy(sellInfoP->accountId, 
          accountId,
          sizeof(sellInfoP->accountId));

  sellInfoP->symbolId = symbolId;
  strncpy(sellInfoP->symbol, 
          symbol,
          sizeof(sellInfoP->symbol));

  sellInfoP->price = price;
  sellInfoP->amount = amount;

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
chronosPackViewPortfolio(const char *accountId,
                         chronosViewPortfolioInfo_t *portfolioInfoP)
{
  int rc = CHRONOS_SUCCESS;

  if (portfolioInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  strncpy(portfolioInfoP->accountId, 
          accountId,
          sizeof(portfolioInfoP->accountId));

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
chronosPackViewStock(int symbolId, 
                     const char *symbol, 
                     chronosSymbol_t *symbolInfoP)
{
  int rc = CHRONOS_SUCCESS;

  if (symbolInfoP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  symbolInfoP->symbolId = symbolId;
  strncpy(symbolInfoP->symbol, 
          symbol,
          sizeof(symbolInfoP->symbol));

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

chronosRequest
chronosRequestCreate(chronosUserTransaction_t txnType, 
                     chronosClientCache  clientCacheH,
                     chronosEnv envH)
{
  int i;
#if 0
  int random_num_data_items = CHRONOS_MIN_DATA_ITEMS_PER_XACT + rand() % (1 + CHRONOS_MAX_DATA_ITEMS_PER_XACT - CHRONOS_MIN_DATA_ITEMS_PER_XACT);
#endif
  int random_num_data_items = 3;
  int rc = CHRONOS_SUCCESS;
  int random_number = 0;
  int random_symbol;
  int random_user;
  int random_amount;
  float random_price;
  const char *symbol;
  const char *user;
  chronosRequestPacket_t *reqPacketP = NULL;
  chronosCache chronosCacheH = NULL;

  if (envH == NULL || clientCacheH == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  chronosCacheH = chronosEnvCacheGet(envH);
  if (chronosCacheH == NULL) {
    chronos_error("Invalid cache handle");
    goto failXit;
  }

  reqPacketP = malloc(sizeof(chronosRequestPacket_t));
  if (reqPacketP == NULL) {
    chronos_error("Could not allocate request structure");
    goto failXit;
  }

  memset(reqPacketP, 0, sizeof(*reqPacketP));
  reqPacketP->txn_type = txnType;
  reqPacketP->numItems = random_num_data_items;

  switch (txnType) {
    case CHRONOS_USER_TXN_VIEW_STOCK:
      random_user = rand() % chronosClientCacheNumPortfoliosGet(clientCacheH);
      for (i=0; i<random_num_data_items; i++) {
        // Choose a random symbol for this user
        random_number = rand() % chronosClientCacheNumSymbolFromUserGet(random_user, clientCacheH);

        // Now get the symbol
        random_symbol = chronosClientCacheSymbolFromUserGet(random_number, random_user, clientCacheH);
        symbol = chronosCacheSymbolGet(random_symbol, chronosCacheH);
        rc = chronosPackViewStock(random_symbol, 
                                   symbol,
                                   &(reqPacketP->request_data.symbolInfo[i]));
        if (rc != CHRONOS_SUCCESS) {
          chronos_error("Could not pack view stock request");
          goto failXit;
        }
      }

      break;

    case CHRONOS_USER_TXN_VIEW_PORTFOLIO:
      for (i=0; i<random_num_data_items; i++) {
        random_user = rand() % chronosClientCacheNumPortfoliosGet(clientCacheH);
        user = chronosCacheUserGet(random_user, chronosCacheH);
        rc = chronosPackViewPortfolio(user,
                                     &(reqPacketP->request_data.portfolioInfo[i]));
        if (rc != CHRONOS_SUCCESS) {
          chronos_error("Could not pack view portfolio request");
          goto failXit;
        }
      }
      break;

    case CHRONOS_USER_TXN_PURCHASE:
      for (i=0; i<random_num_data_items; i++) {
        // Choose a random user
        random_number = rand() % chronosClientCacheNumPortfoliosGet(clientCacheH);

        // Get user details
        random_user = chronosClientCacheUserIdGet(random_number, clientCacheH);
        user = chronosCacheUserGet(random_user, chronosCacheH);

        // Choose a random symbol for this user
        random_number = rand() % chronosClientCacheNumSymbolFromUserGet(random_user, clientCacheH);

        // Now get the symbol
        random_symbol = chronosClientCacheSymbolFromUserGet(random_user, random_number, clientCacheH);
        symbol = chronosCacheSymbolGet(random_symbol, chronosCacheH);

        random_amount = rand() % 100;
        random_price = chronosClientCacheSymbolPriceFromUserGet(random_user, random_number, clientCacheH) - 0.1;

        rc = chronosPackPurchase(user,
                                 random_symbol, symbol,
                                 random_price, random_amount,
                                 &(reqPacketP->request_data.purchaseInfo[i]));
        if (rc != CHRONOS_SUCCESS) {
          chronos_error("Could not pack purchase request");
          goto failXit;
        }
      }
      break;

    case CHRONOS_USER_TXN_SALE:
      for (i=0; i<random_num_data_items; i++) {
        // Choose a random user
        random_number = rand() % chronosClientCacheNumPortfoliosGet(clientCacheH);

        // Get user details
        random_user = chronosClientCacheUserIdGet(random_number, clientCacheH);
        user = chronosCacheUserGet(random_user, chronosCacheH);

        // Choose a random symbol for this user
        random_number = rand() % chronosClientCacheNumSymbolFromUserGet(random_user, clientCacheH);

        // Now get the symbol
        random_symbol = chronosClientCacheSymbolFromUserGet(random_user, random_number, clientCacheH);
        symbol = chronosCacheSymbolGet(random_symbol, chronosCacheH);

        random_amount = rand() % 100;
        random_price = chronosClientCacheSymbolPriceFromUserGet(random_user, random_number, clientCacheH) + 0.1;

        rc = chronosPackSellStock(user,
                                  random_symbol, symbol,
                                  random_price, random_amount,
                                  &(reqPacketP->request_data.sellInfo[i]));
        if (rc != CHRONOS_SUCCESS) {
          chronos_error("Could not pack sell request");
          goto failXit;
        }
      }
      break;

    default:
      assert("Invalid transaction type" == 0);
  }

  goto cleanup;

failXit:
  if (reqPacketP != NULL) {
    free(reqPacketP);
    reqPacketP = NULL;
  }

cleanup:
  return (void *) reqPacketP;
}

int
chronosRequestFree(chronosRequest requestH)
{
  chronosRequestPacket_t *requestP = NULL;

  if (requestH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  requestP = (chronosRequestPacket_t *) requestH;
  memset(requestP, 0, sizeof(*requestP));
  free(requestP);

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

chronosUserTransaction_t
chronosRequestTypeGet(chronosRequest requestH)
{
  chronosRequestPacket_t *requestP = NULL;

  if (requestH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  requestP = (chronosRequestPacket_t *) requestH;
  return requestP->txn_type;

failXit:
  return CHRONOS_USER_TXN_INVAL;
}

size_t
chronosRequestSizeGet(chronosRequest requestH)
{
  chronosRequestPacket_t *requestP = NULL;

  if (requestH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  requestP = (chronosRequestPacket_t *) requestH;
  return sizeof(*requestP);

failXit:
  return -1;
}

chronosResponse
chronosResponseAlloc()
{
  chronosResponsePacket_t *resPacketP = NULL;

  resPacketP = malloc(sizeof(chronosResponsePacket_t));
  if (resPacketP == NULL) {
    chronos_error("Could not allocate response structure");
    goto failXit;
  }

  memset(resPacketP, 0, sizeof(*resPacketP));
  goto cleanup;

failXit:
  if (resPacketP != NULL) {
    free(resPacketP);
    resPacketP = NULL;
  }

cleanup:
  return (void *) resPacketP;
}

int
chronosResponseFree(chronosResponse responseH)
{
  chronosResponsePacket_t *responseP = NULL;

  if (responseH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  responseP = (chronosResponsePacket_t *) responseH;
  memset(responseP, 0, sizeof(*responseP));
  free(responseP);

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

size_t
chronosResponseSizeGet(chronosResponse responseH)
{
  chronosResponsePacket_t *responseP = NULL;

  if (responseH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  responseP = (chronosResponsePacket_t *) responseH;
  return sizeof(*responseP);

failXit:
  return -1;
}

chronosUserTransaction_t
chronosResponseTypeGet(chronosResponse responseH)
{
  chronosResponsePacket_t *responseP = NULL;

  if (responseH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  responseP = (chronosResponsePacket_t *) responseH;
  return responseP->txn_type;

failXit:
  return CHRONOS_USER_TXN_INVAL;
}

int
chronosResponseResultGet(chronosResponse responseH)
{
  chronosResponsePacket_t *responseP = NULL;

  if (responseH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  responseP = (chronosResponsePacket_t *) responseH;
  return responseP->rc;

failXit:
  return CHRONOS_USER_TXN_INVAL;
}

