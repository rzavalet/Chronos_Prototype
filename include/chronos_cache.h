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

#endif
