typedef struct chronosRequestPacket_t {
  chronos_user_transaction_t txn_type;
} chronosRequestPacket_t;

typedef struct chronosResponsePacket_t {
  chronos_user_transaction_t txn_type;
  int rc;
} chronosResponsePacket_t;
