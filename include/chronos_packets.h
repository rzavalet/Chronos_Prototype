#ifndef _CHRONOS_PACKETS_H_
#define _CHRONOS_PACKETS_H_

#include "chronos_transactions.h"

#define   ID_SZ           10
typedef struct chronosRequestPacket_t {
  chronos_user_transaction_t txn_type;
  int  symbolId;
  char symbol[ID_SZ];
} chronosRequestPacket_t;

typedef struct chronosResponsePacket_t {
  chronos_user_transaction_t txn_type;
  int rc;
} chronosResponsePacket_t;


#endif
