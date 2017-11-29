#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "chronos.h"
#include "chronos_environment.h"

#define CHRONOS_ENV_MAGIC   (0xCAFE)

typedef struct chronosEnv_t {
  int          magic;
  chronosCache cacheH;
} chronosEnv_t;

chronosCache
chronosEnvCacheGet(chronosEnv envH)
{
  int rc = CHRONOS_SUCCESS;
  chronosEnv_t *envP = NULL;

  if (envH == NULL) {
    chronos_error("Invalid env handle");
    goto failXit;
  }

  rc = chronosEnvCheck(envH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Bad env handle");
    goto failXit;
  }

  envP = (chronosEnv_t *) envH;

  return envP->cacheH;

failXit:
  return NULL;
}

int
chronosEnvCheck(chronosEnv envH)
{
  int rc = CHRONOS_SUCCESS;
  chronosEnv_t *envP = NULL;

  if (envH == NULL) {
    chronos_error("Invalid env handle");
    goto failXit;
  }

  envP = (chronosEnv_t *) envH;

  assert(envP->magic == CHRONOS_ENV_MAGIC);

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

chronosEnv
chronosEnvAlloc(const char *homedir, 
                const char *datafilesdir)
{
  chronosEnv_t *envP = NULL;
  chronosCache  cacheH = NULL;

  envP = malloc(sizeof(chronosEnv_t));
  if (envP == NULL) {
    chronos_error("Could not allocate env structure");
    goto failXit;
  }

  memset(envP, 0, sizeof(*envP));

  cacheH = chronosCacheAlloc(homedir, datafilesdir);
  if (cacheH == NULL) {
    chronos_error("Failed to create cache");
    goto failXit;    
  }
  envP->cacheH = cacheH;
  envP->magic = CHRONOS_ENV_MAGIC;

  goto cleanup;

failXit:
  if (cacheH != NULL) {
    chronosCacheFree(cacheH);
    cacheH = NULL;
  }

  if (envP != NULL) {
    free(envP);
    envP = NULL;
  }
  
cleanup:
  return  (void *) envP;
}

int
chronosEnvFree(chronosEnv envH) 
{
  int rc = CHRONOS_SUCCESS;
  chronosEnv_t *envP = NULL;

  if (envH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  rc = chronosEnvCheck(envH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Bad env handle");
    goto failXit;
  }
  
  envP = (chronosEnv_t *) envH;

  rc = chronosCacheFree(envP->cacheH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Could not free env items");
    goto failXit;
  }

  memset(envP, 0, sizeof(*envP));
  free(envP);

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

