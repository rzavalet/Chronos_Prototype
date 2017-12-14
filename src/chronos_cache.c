#include <stdio.h>
#include <stdlib.h>
#include "chronos_cache.h"
#include "chronos.h"
#include "benchmark_common.h"

#define CHRONOS_CLIENT_NUM_STOCKS     (300)
#define CHRONOS_CLIENT_NUM_USERS      (50)
#define MAXLINE   1024

typedef struct chronosPortfolioCache_t {
  char    *symbol;
  int     random_amount;
  float   random_price;
} chronosPortfolioCache_t;

typedef struct chronosClientCache_t {
  char                    *user;
  int                     numPortfolios;
  chronosPortfolioCache_t portfolio[100];
} chronosClientCache_t;

typedef struct chronosCache_t {
  char                **stocksListP;
  char                users[CHRONOS_CLIENT_NUM_USERS][256];

  int                 numStocks;
  int                 numUsers;

  chronosClientCache_t    clientCache[100];
} chronosCache_t;

#define MIN(a,b)        (a < b ? a : b)

static int
createPortfolios(int numClients, chronosCache_t *cacheP)
{
  int i, j;
  int numSymbols = 0;
  int numUsers =  0;
  int numClients = 0;
  int usersPerClient = 0;
  int symbolsPerUser = 0;
  int random_symbol;
  int random_user;
  int random_amount;
  float random_price;
  chronosClientCache_t  *clientCacheP = NULL;

  if (cacheP == NULL) {
    chronos_error("Invalid cache pointer");
    goto failXit;
  }

  clientCacheP = &cacheP->clientCache;
  numSymbols = chronosCacheNumSymbolsGet(chronosCacheH);
  numUsers = chronosCacheNumUsersGet(chronosCacheH);

  usersPerClient = MIN(numUsers / numClients, 100);
  symbolsPerUser = MIN(numSymbols / numUsers, 100);

  cacheP->numCachedClients = usersPerClient;
  for (i=0; i<usersPerClient; i++) {
    random_user = i + (usersPerClient * (clientCacheP->thread_num -1));
    clientCacheP[i].user = chronosCacheUserGet(random_user, chronosCacheH);
    clientCacheP[i].numPortfolios = symbolsPerUser;

    for (j=0; j<symbolsPerUser; j++) {
      random_symbol = rand() % chronosCacheNumSymbolsGet(chronosCacheH);
      random_amount = rand() % 100;
      random_price = rand() % 1000;

      clientCacheP[i]->portfolio[j].symbol = chronosCacheSymbolGet(random_symbol, chronosCacheH);
      clientCacheP[i]->portfolio[j].random_amount = random_amount;
      clientCacheP[i]->portfolio[j].random_price = random_price;
    }
  }

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL;
}

chronosClientCache
chronosClientCacheAlloc(int numThreads, int numClients, chronosCache chronosCacheH)
{
  int i;
  chronosCache_t *cacheP = NULL;
  chronosClientCache_t *clientCacheP = NULL;
  int rc = CHRONOS_SUCCESS;

  if (chronosCacheH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  cacheP = (chronosCache_t *) chronosCacheH;

  clientCacheP = malloc(sizeof(chronosClientCache_t));
  if (clientCacheP == NULL) {
    chronos_error("Could not allocate cache structure");
    goto failXit;
  }

  memset(clientCacheP, 0, sizeof(*clientCacheP));

  rc = createPortfolios(numClients, cacheP);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not populate client cache");
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

