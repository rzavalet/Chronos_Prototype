#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "chronos.h"
#include "chronos_environment.h"
#include "chronos_client.h"

typedef struct chronosClientConnection_t {
  char                connectionName[256];
  char                serverAddress[256];
  int                 serverPort;
  int                 socket_fd;
  chronosConnState_t  state;
  chronosEnv          envH; 
} chronosClientConnection_t;

chronosEnv
chronosClientEnvGet(chronosConnHandle connH)
{
  int rc = CHRONOS_SUCCESS;
  chronosClientConnection_t *connectionP = NULL;

  if (connH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  connectionP = (chronosClientConnection_t *) connH;

  rc = chronosEnvCheck(connectionP->envH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Bad env handle");
    goto failXit;
  }
  
  return connectionP->envH;

failXit:
  return NULL;
}

int
chronosClientDisconnect(chronosConnHandle connH)
{
  chronosClientConnection_t *connectionP = NULL;

  if (connH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  connectionP = (chronosClientConnection_t *) connH;

  if (connectionP->state == CHRONOS_CONNECTION_CONNECTED) {
    close(connectionP->socket_fd);
    connectionP->socket_fd = -1;
  }

  connectionP->state = CHRONOS_CONNECTION_DISCONNECTED;

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

int
chronosClientConnect(const char *serverAddress,
                     int serverPort,
                     const char *connName,
                     chronosConnHandle connH) 
{
  int on = 1;
  int socket_fd;
  int rc = CHRONOS_SUCCESS;
  chronosClientConnection_t *connectionP = NULL;
  struct sockaddr_in chronos_server_address;
  struct pollfd fds[1];

  if (connH == NULL) {
    chronos_error("Invalid connection handle");
    goto failXit;
  }

  if (serverPort == 0 || serverAddress == NULL) {
    chronos_error("Invalid arguments");
    goto failXit;
  }

  connectionP = (chronosClientConnection_t *) connH;

  if (connectionP->state != CHRONOS_CONNECTION_DISCONNECTED) {
    chronos_error("Invalid connection state");
    goto failXit;
  }

  strncpy(connectionP->serverAddress, 
          serverAddress, 
          sizeof(connectionP->serverAddress));
  connectionP->serverPort = serverPort;

  if (connName != NULL) {
    strncpy(connectionP->connectionName, 
            connName, 
            sizeof(connectionP->connectionName));
  }
  else {
    snprintf(connectionP->connectionName,
             sizeof(connectionP->connectionName),
             "%s:%d", 
             serverAddress,
             serverPort);
  }

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    perror("socket() failed");
    goto failXit;
  }

  /* Make socket reusable */
  rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  if (rc == -1) {
    perror("setsockopt() failed");
    goto failXit;
  }

  /* Make non-blocking socket */
  rc = ioctl(socket_fd, FIONBIO, (char *)&on);
  if (rc < 0) {
    perror("ioctl() failed");
    goto failXit;
  }

  fds[0].fd = socket_fd;
  fds[0].events = POLLOUT;

  chronos_server_address.sin_family = AF_INET;
  chronos_server_address.sin_addr.s_addr = inet_addr(serverAddress);
  chronos_server_address.sin_port = htons(serverPort);
  
  rc = connect(socket_fd, 
                (struct sockaddr *)&chronos_server_address, 
                sizeof(chronos_server_address));
  if (rc < 0 && errno != EINPROGRESS) {
    perror("connect() failed");
    goto failXit;
  }
  else if (rc == 0) {
    /* connected successfully */
  }
  else {
    /* wait for connect to complete */
    while (fds[0].events) {
      int result;
      socklen_t result_len = sizeof(result);

      rc = poll(fds, 1, 1000 /* one second */);
      if (rc < 0) {
        perror("poll() failed");
        goto failXit;
      }

      /* poll timed out */
      if (rc == 0) {
        continue;
      }

      rc = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &result, &result_len);
      if (rc < 0) {
        perror("getsockopt() failed");
        goto failXit; 
      }

      if (result != 0) {
        char errMsg[256];
        (void)strerror_r(result, errMsg, sizeof(errMsg));
        chronos_error("connect() failed: %s", errMsg);
        goto failXit;
      }
    
      break;
    }
  }

  connectionP->socket_fd = socket_fd;

  connectionP->state = CHRONOS_CONNECTION_CONNECTED;

  return CHRONOS_SUCCESS;

failXit:
  
  connectionP->socket_fd = -1;
  connectionP->state = CHRONOS_CONNECTION_DISCONNECTED;
  return CHRONOS_FAIL; 
}

chronosConnHandle
chronosConnHandleAlloc(chronosEnv envH)
{
  int rc = CHRONOS_SUCCESS;
  chronosClientConnection_t *connectionP = NULL;

  connectionP = malloc(sizeof(chronosClientConnection_t));
  if (connectionP == NULL) {
    chronos_error("Could not allocate connection handle");
    goto failXit;
  }

  memset(connectionP, 0, sizeof(*connectionP));

  rc = chronosEnvCheck(envH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Bad env handle");
    goto failXit;
  }

  connectionP->envH = envH;
  connectionP->state = CHRONOS_CONNECTION_DISCONNECTED;
  goto cleanup;

failXit:
  if (connectionP != NULL) {
    free(connectionP);
    connectionP = NULL;
  }
  
cleanup:
  return  (void *) connectionP;
}

int
chronosConnHandleFree(chronosConnHandle connH) 
{
  int rc = CHRONOS_SUCCESS;
  chronosClientConnection_t *connectionP = NULL;

  if (connH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  connectionP = (chronosClientConnection_t *) connH;

  if (connectionP->state != CHRONOS_CONNECTION_DISCONNECTED) {
    chronos_error("Invalid state");
    goto failXit;
  }

  rc = chronosEnvCheck(connectionP->envH);
  if (rc != CHRONOS_SUCCESS) {
    chronos_error("Bad env handle");
    goto failXit;
  }
  
  memset(connectionP, 0, sizeof(*connectionP));
  free(connectionP);

  goto cleanup;

failXit:
  rc = CHRONOS_FAIL;

cleanup:
  return rc;
}

/*
 * Sends a transaction request to the Chronos Server
 */
int
chronosClientSendRequest(chronosRequest    requestH,
                         chronosConnHandle connH)
{
  int written, to_write;
  char *buf = NULL;
  chronosClientConnection_t *connectionP = NULL;

  if (connH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  if (requestH == NULL) {
    chronos_error("Invalid packet");
    goto failXit;
  }

  connectionP = (chronosClientConnection_t *) connH;

  if (connectionP->state != CHRONOS_CONNECTION_CONNECTED) {
    chronos_error("Invalid connection state");
    goto failXit;
  }

  chronos_info("Sending new transaction request: %d", 
                chronosRequestTypeGet(requestH));

  buf = (char *)requestH;
  to_write = chronosRequestSizeGet(requestH);
 
  while(to_write >0) {
    written = write(connectionP->socket_fd, buf, to_write);
    if (written < 0) {
      chronos_error("Failed to write to socket");
      goto failXit;
    }

    to_write -= written;
    buf += written;
  }

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

/* 
 * Waits for response from chronos server
 */
int
chronosClientReceiveResponse(chronosConnHandle connH, 
                             int (*isTimeToDieFp) (void))
{
  int rc;
  int num_bytes;
  chronosResponse responseH = NULL; 
  chronosClientConnection_t *connectionP = NULL;
  struct pollfd fds[1];

  if (connH == NULL) {
    chronos_error("Invalid handle");
    goto failXit;
  }

  connectionP = (chronosClientConnection_t *) connH;

  if (connectionP->state != CHRONOS_CONNECTION_CONNECTED) {
    chronos_error("Invalid connection state");
    goto failXit;
  }

  responseH = chronosResponseAlloc();
  if (responseH == NULL) {
    chronos_error("Could not create response");
    goto failXit;
  }

  fds[0].fd = connectionP->socket_fd;
  fds[0].events = POLLIN;

  /* While we are interested in reading from this 
   * socket 
   */
  while(fds[0].events) {
    char *buf = (char *) responseH;
    int   to_read = chronosResponseSizeGet(responseH);

    if (isTimeToDieFp()) {
      chronos_error("requested to die");
      goto failXit;
    }

    rc = poll(fds, 1, 1000 /* one second */);
    if (rc < 0) {
      perror("poll() failed");
      goto failXit;
    }
    else if (rc == 0) {
      chronos_error("poll() timed out");
      continue;
    }

    assert(fds[0].revents);

    while (to_read > 0) {
      num_bytes = read(connectionP->socket_fd, buf, 1);
      if (num_bytes < 0) {
        perror("read() failed");
        goto failXit;
      }
      else if (num_bytes == 0) {
        chronos_error("socket closed");
        fds[0].events = 0;
        goto failXit;
      }

      to_read -= num_bytes;
      buf += num_bytes;
    }

    chronos_info("Txn: %d, rc: %d", 
                  chronosResponseTypeGet(responseH),
                  chronosResponseResultGet(responseH));
    break;
  }

  return CHRONOS_SUCCESS;

failXit:
  return CHRONOS_FAIL; 
}

