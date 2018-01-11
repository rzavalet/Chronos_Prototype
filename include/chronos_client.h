#ifndef _CHRONOS_CLIENT_H_
#define _CHRONOS_CLIENT_H_

#include "chronos_packets.h"
#include "chronos_environment.h"

typedef enum {
  CHRONOS_CONNECTION_INVALID = 0,
  CHRONOS_CONNECTION_DISCONNECTED,
  CHRONOS_CONNECTION_CONNECTED
} chronosConnState_t;

typedef void *chronosConnHandle;

chronosEnv
chronosClientEnvGet(chronosConnHandle connH);

chronosConnHandle
chronosConnHandleAlloc(chronosEnv envH);

int
chronosConnHandleFree(chronosConnHandle connH);

int
chronosClientConnect(const char *serverAddress,
                     int serverPort,
                     const char *connName,
                     chronosConnHandle connH);

int
chronosClientDisconnect(chronosConnHandle connH);

int
chronosClientSendRequest(chronosRequest    requestH,
                         chronosConnHandle connH);

int
chronosClientReceiveResponse(int *txn_rc_ret, 
                             chronosConnHandle connH, 
                             int (*isTimeToDieFp) (void));

#endif
