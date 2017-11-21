#ifndef _CHRONOS_PACKETS_H_
#define _CHRONOS_PACKETS_H_

#include "chronos_config.h"
#include "chronos_transactions.h"

typedef struct chronosRequestPacket_t {
  chronos_user_transaction_t txn_type;

  /* A transaction can affect up to 100 symbols */
  int numSymbols;
  union {
    chronosViewPortfolioInfo_t portfolioInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
    chronosSymbol_t            symbolInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
    chronosPurchaseInfo_t      purchaseInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
    chronosSellInfo_t          sellInfo[CHRONOS_MAX_DATA_ITEMS_PER_XACT];
  } request_data;

} chronosRequestPacket_t;

typedef struct chronosResponsePacket_t {
  chronos_user_transaction_t txn_type;
  int rc;
} chronosResponsePacket_t;


int
chronosPackPurchase(const char *accountId,
                    int          symbolId, 
                    const char *symbol, 
                    float        price,
                    int          amount,
                    chronosPurchaseInfo_t *purchaseInfoP);

int
chronosPackSellStock(const char *accountId,
                     int          symbolId, 
                     const char *symbol, 
                     float        price,
                     int          amount,
                     chronosSellInfo_t *sellInfoP);

int
chronosPackViewPortfolio(const char *accountId,
                         chronosViewPortfolioInfo_t *portfolioInfoP);

int
chronosPackViewStock(int symbolId, 
                     const char *symbol, 
                     chronosSymbol_t *symbolInfoP);
#endif
