#ifndef _CHRONOS_TRANSACTIONS_H_
#define _CHRONOS_TRANSACTIONS_H_

/*---------------------------------
 * Important ENUMS
 *-------------------------------*/
typedef enum chronos_user_transaction_t {
  CHRONOS_USER_TXN_MIN = 0,
  CHRONOS_USER_TXN_VIEW_STOCK = CHRONOS_USER_TXN_MIN,
  CHRONOS_USER_TXN_VIEW_PORTFOLIO,
  CHRONOS_USER_TXN_PURCHASE,
  CHRONOS_USER_TXN_SALE,
  CHRONOS_USER_TXN_MAX,
  CHRONOS_USER_TXN_INVAL=CHRONOS_USER_TXN_MAX
} chronos_user_transaction_t;

typedef enum chronos_system_transaction_t {
  CHRONOS_SYS_TXN_MIN = 0,
  CHRONOS_SYS_TXN_UPDATE_STOCK = CHRONOS_SYS_TXN_MIN,
  CHRONOS_SYS_TXN_MAX,
  CHRONOS_SYS_TXN_INVAL=CHRONOS_SYS_TXN_MAX
} chronos_system_transaction_t;

#endif
