#ifndef _CHRONOS_ENVIRONMENT_H_
#define _CHRONOS_ENVIRONMENT_H_

#include "chronos_cache.h"

typedef void *chronosEnv;

chronosEnv
chronosEnvAlloc(const char *homedir, 
                const char *datafilesdir);

int
chronosEnvFree(chronosEnv envH);

int
chronosEnvCheck(chronosEnv envH);

chronosCache
chronosEnvCacheGet(chronosEnv envH);
#endif

