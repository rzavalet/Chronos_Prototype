#include <stdio.h>
#include <stdlib.h>
#include "chronos_cache.h"
#include "chronos.h"
#include "benchmark_common.h"

#define CHRONOS_CLIENT_NUM_STOCKS     (300)
#define CHRONOS_CLIENT_NUM_USERS      (50)
#define MAXLINE   1024

/*
 * Information for each symbol a user is interested in.
 */
typedef struct chronosClientStockInfo_t
{
  int     symbolId;
  const char    *symbol;
  int     random_amount;
  float   random_price;
} chronosClientStockInfo_t;

/*
 * A user's portfolio information.
 * A user is interested in k stock symbols.
 */
typedef struct chronosClientPortfolios_t 
{
  int         userId;

  /* Which user is this? */
  const char *user;

  /* How many symbols the user is interested in */
  int   numSymbols;

  /* Information for each of the k stocks managed by a user */
  chronosClientStockInfo_t  stockInfoArr[100];
} chronosClientPortfolios_t;

typedef struct chronosClientCache_t 
{
  int                     numPortfolios;

  /*List of portfolios handled by this client thread:
   * we have one entry in the array per each managed user
   */
  chronosClientPortfolios_t portfoliosArr[100];
} chronosClientCache_t;

typedef struct chronosCache_t {
  int                 numStocks;
  char                **stocksListP;

  int                 numUsers;
  char                users[CHRONOS_CLIENT_NUM_USERS][256];
} chronosCache_t;


#define MIN(a,b)        (a < b ? a : b)
#define MAX(a,b)        (a > b ? a : b)

/* This client process will handle n users.
 * So, for each user, we need to create its portfolio.
 */
static int
createPortfolios(int numClient, int numClients, chronosClientCache_t *clientCacheP, chronosCache chronosCacheH)
{
  int i, j;
  int numSymbols = 0;
  int numUsers =  0;
  int numPortfolios = 0;
  int symbolsPerUser = 0;
  int random_symbol;
  int random_user;
  int random_amount;
  float random_price;

  if (chronosCacheH == NULL || clientCacheP == NULL) {
    chronos_error("Invalid cache pointer");
    goto failXit;
  }

  numSymbols = chronosCacheNumSymbolsGet(chronosCacheH);
  numUsers = chronosCacheNumUsersGet(chronosCacheH);

  numPortfolios = MAX(MIN(numUsers / numClients, 100), 10);
  symbolsPerUser = MAX(MIN(numSymbols / numUsers, 100), 10);

  fprintf(stderr, "DEBUG: numSymbols: %d, numUsers: %d, numPortfolios: %d, symbolsPerClient: %d\n",
                  numSymbols,
                  numUsers,
                  numPortfolios,
                  symbolsPerUser);

  clientCacheP->numPortfolios = numPortfolios;

  for (i=0; i<numPortfolios; i++) {
    /* TODO: does it matter which client we choose? */
    random_user = (i + (numPortfolios * (numClient -1))) % numUsers;

    clientCacheP->portfoliosArr[i].userId = random_user;
    clientCacheP->portfoliosArr[i].user = chronosCacheUserGet(random_user, chronosCacheH);
    clientCacheP->portfoliosArr[i].numSymbols = symbolsPerUser;

    for (j=0; j<symbolsPerUser; j++) {
      random_symbol = rand() % chronosCacheNumSymbolsGet(chronosCacheH);
      random_amount = rand() % 100;
      random_price = 500.0;

      clientCacheP->portfoliosArr[i].stockInfoArr[j].symbolId = random_symbol;
      clientCacheP->portfoliosArr[i].stockInfoArr[j].symbol = chronosCacheSymbolGet(random_symbol, chronosCacheH);
      clientCacheP->portfoliosArr[i].stockInfoArr[j].random_amount = random_amount;
      clientCacheP->portfoliosArr[i].stockInfoArr[j].random_price = random_price;
      fprintf(stderr, "DEBUG: Client %d Handling user: %s symbol: %s\n",
                      numClient,
                      clientCacheP->portfoliosArr[i].user,
                      clientCacheP->portfoliosArr[i].stockInfoArr[j].symbol);
    }
  }

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

chronosClientCache
chronosClientCacheAlloc(int numClient, int numClients, chronosCache chronosCacheH)
{
  chronosClientCache_t *clientCacheP = NULL;
  int rc = CHRONOS_SUCCESS;

  if (chronosCacheH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  clientCacheP = malloc(sizeof(chronosClientCache_t));
  if (clientCacheP == NULL) {
    chronos_error("Could not allocate cache structure");
    goto failXit;
  }

  memset(clientCacheP, 0, sizeof(*clientCacheP));

  rc = createPortfolios(numClient, numClients, clientCacheP, chronosCacheH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Failed to create porfolios cache");
    goto failXit;
  }

  goto cleanup;

failXit:
  if (clientCacheP != NULL) {
    free(clientCacheP);
    clientCacheP = NULL;
  }
  
cleanup:
  return  (void *) clientCacheP;
}

int
chronosClientCacheFree(chronosClientCache chronosClientCacheH)
{
  int rc = CHRONOS_SUCCESS;
  chronosClientCache_t *cacheP = NULL;

  if (chronosClientCacheH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  cacheP = (chronosClientCache_t *) chronosClientCacheH;
  memset(cacheP, 0, sizeof(*cacheP));

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

int
chronosClientCacheNumPortfoliosGet(chronosClientCache  clientCacheH)
{
  chronosClientCache_t *clientCacheP = NULL;

  if (clientCacheH == NULL) {
    return 0;
  }

  clientCacheP = (chronosClientCache_t *) clientCacheH;
  return clientCacheP->numPortfolios;
}

int
chronosClientCacheUserIdGet(int numUser, chronosClientCache  clientCacheH)
{
  chronosClientCache_t *clientCacheP = NULL;

  if (clientCacheH == NULL) {
    return 0;
  }

  clientCacheP = (chronosClientCache_t *) clientCacheH;
  assert(0 <= numUser && numUser < clientCacheP->numPortfolios);

  return clientCacheP->portfoliosArr[numUser].userId;
}

int
chronosClientCacheNumSymbolFromUserGet(int numUser, chronosClientCache  clientCacheH)
{
  chronosClientCache_t *clientCacheP = NULL;

  if (clientCacheH == NULL) {
    return 0;
  }

  clientCacheP = (chronosClientCache_t *) clientCacheH;
  assert(0 <= numUser && numUser < clientCacheP->numPortfolios);

  return clientCacheP->portfoliosArr[numUser].numSymbols;
}

int
chronosClientCacheSymbolFromUserGet(int numUser, int numSymbol, chronosClientCache  clientCacheH)
{
  chronosClientCache_t *clientCacheP = NULL;

  if (clientCacheH == NULL) {
    return 0;
  }

  clientCacheP = (chronosClientCache_t *) clientCacheH;
  assert(0 <= numUser && numUser < clientCacheP->numPortfolios);
  assert(0 <= numSymbol && numSymbol < clientCacheP->portfoliosArr[numUser].numSymbols);

  return clientCacheP->portfoliosArr[numUser].stockInfoArr[numSymbol].symbolId;
}

float
chronosClientCacheSymbolPriceFromUserGet(int numUser, int numSymbol, chronosClientCache  clientCacheH)
{
  chronosClientCache_t *clientCacheP = NULL;

  if (clientCacheH == NULL) {
    return 0;
  }

  clientCacheP = (chronosClientCache_t *) clientCacheH;
  assert(0 <= numUser && numUser < clientCacheP->numPortfolios);
  assert(0 <= numSymbol && numSymbol < clientCacheP->portfoliosArr[numUser].numSymbols);

  return clientCacheP->portfoliosArr[numUser].stockInfoArr[numSymbol].random_price;
}

static int
stockListFree(chronosCache_t *cacheP) 
{
  int rc = CHRONOS_SUCCESS;
  int i;

  if (cacheP == NULL) {
    chronos_error("Invalid argument");
    goto failXit;
  }

  if (cacheP->stocksListP != NULL) {
    for (i=0; i<CHRONOS_CLIENT_NUM_STOCKS; i++) {
      if (cacheP->stocksListP[i] != NULL) {
        free(cacheP->stocksListP[i]);
        cacheP->stocksListP[i] = NULL;
      }
    }

    free(cacheP->stocksListP);
    cacheP->stocksListP = NULL;
  }

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

static int
stockListFromFile(const char      *homedir, 
                  const char      *datafilesdir, 
                  chronosCache_t  *cacheP)
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
  
  if (cacheP == NULL) {
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
  while (fgets(buf, MAXLINE, ifp) != NULL 
          && current_slot < CHRONOS_CLIENT_NUM_STOCKS) {

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
  if (cacheP != NULL) {
    cacheP->stocksListP = listP;
  }

  return rc;
}


chronosCache
chronosCacheAlloc(const char *homedir, 
                  const char *datafilesdir)
{
  int i;
  chronosCache_t *cacheP = NULL;
  int rc = CHRONOS_SUCCESS;

  cacheP = malloc(sizeof(chronosCache_t));
  if (cacheP == NULL) {
    chronos_error("Could not allocate cache structure");
    goto failXit;
  }

  memset(cacheP, 0, sizeof(*cacheP));

  rc = stockListFromFile(homedir, datafilesdir, cacheP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not initialize cache structure");
    goto failXit;
  }

  cacheP->numStocks = CHRONOS_CLIENT_NUM_STOCKS;

  cacheP->numUsers = CHRONOS_CLIENT_NUM_USERS;
  for (i=0; i<CHRONOS_CLIENT_NUM_USERS; i++) {
    snprintf(cacheP->users[i], sizeof(cacheP->users[i]),
             "%d", i);
  }

  goto cleanup;

failXit:
  if (cacheP != NULL) {
    free(cacheP);
    cacheP = NULL;
  }
  
cleanup:
  return  (void *) cacheP;
}

int
chronosCacheFree(chronosCache chronosCacheH)
{
  int rc = CHRONOS_SUCCESS;
  chronosCache_t *cacheP = NULL;

  if (chronosCacheH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  cacheP = (chronosCache_t *) chronosCacheH;

  rc = stockListFree(cacheP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not free cached items");
    goto failXit;
  }

  memset(cacheP, 0, sizeof(*cacheP));

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}


int
chronosCacheNumSymbolsGet(chronosCache chronosCacheH)
{
  chronosCache_t *cacheP= NULL;

  if (chronosCacheH == NULL) {
    return 0;
  }

  cacheP = (chronosCache_t *) chronosCacheH;
  return cacheP->numStocks;
}

const char *
chronosCacheSymbolGet(int symbolNum,
                      chronosCache chronosCacheH)
{
  chronosCache_t *cacheP= NULL;

  if (chronosCacheH == NULL) {
    chronos_error("Invalid handle");
    return NULL;
  }

  cacheP = (chronosCache_t *) chronosCacheH;

  if (symbolNum < 0 || symbolNum > cacheP->numStocks) {
    return NULL;
  }

  return cacheP->stocksListP[symbolNum];
}

int
chronosCacheNumUsersGet(chronosCache chronosCacheH)
{
  chronosCache_t *cacheP= NULL;

  if (chronosCacheH == NULL) {
    return 0;
  }

  cacheP = (chronosCache_t *) chronosCacheH;
  return cacheP->numUsers;
}

const char *
chronosCacheUserGet(int userNum,
                    chronosCache chronosCacheH)
{
  chronosCache_t *cacheP= NULL;

  if (chronosCacheH == NULL) {
    chronos_error("Invalid handle");
    return NULL;
  }

  cacheP = (chronosCache_t *) chronosCacheH;

  if (userNum < 0 || userNum > cacheP->numUsers) {
    return NULL;
  }

  return cacheP->users[userNum];
}

