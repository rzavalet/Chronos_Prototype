#ifndef _CHRONOS_CACHE_H_
#define _CHRONOS_CACHE_H_

typedef void *chronosCache;

chronosCache
chronosCacheAlloc(const char *homedir, 
                  const char *datafilesdir);

int
chronosCacheFree(chronosCache chronosCacheH);

int
chronosCacheNumSymbolsGet(chronosCache chronosCacheH);

const char *
chronosCacheSymbolGet(int symbolNum,
                      chronosCache chronosCacheH);

int
chronosCacheNumUsersGet(chronosCache chronosCacheH);

const char *
chronosCacheUserGet(int userNum,
                    chronosCache chronosCacheH);

typedef void* chronosClientCache;

chronosClientCache
chronosClientCacheAlloc(int numClient, int numClients, chronosCache chronosCacheH);

int
chronosClientCacheFree(chronosClientCache chronosClientCacheH);

int
chronosClientCacheNumPortfoliosGet(chronosClientCache  clientCacheH);

int
chronosClientCacheUserIdGet(int numUser, chronosClientCache  clientCacheH);

int
chronosClientCacheNumSymbolFromUserGet(int numUser, chronosClientCache  clientCacheH);

int
chronosClientCacheSymbolFromUserGet(int numUser, int numSymbol, chronosClientCache  clientCacheH);

float
chronosClientCacheSymbolPriceFromUserGet(int numUser, int numSymbol, chronosClientCache  clientCacheH);
#endif
