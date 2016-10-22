/*===================================
 * This is the MAIN chronos header file
 * Include this one in any file where
 * you want to call the chronos API
 *==================================*/



/*---------------------------------
 * Important CONSTANTS 
 *-------------------------------*/
#define CHRONOS_SUCCESS         0
#define CHRONOS_FAIL            1



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

const char *chronos_user_transaction_str[] = {
  "CHRONOS_USER_TXN_VIEW_STOCK",
  "CHRONOS_USER_TXN_VIEW_PORTFOLIO",
  "CHRONOS_USER_TXN_PURCHASE",
  "CHRONOS_USER_TXN_SALE"
};

#define CHRONOS_TXN_NAME(_txn_type) \
  ((CHRONOS_USER_TXN_MIN<=(_txn_type) && (_txn_type) < CHRONOS_USER_TXN_MAX) ? (chronos_user_transaction_str[(_txn_type)]) : "INVALID")

/*---------------------------------
 * Debugging routines
 *-------------------------------*/
#define CHRONOS_DEBUG_LEVEL_MIN   (0)
#define CHRONOS_DEBUG_LEVEL_MAX   (10)

int chronos_debug_level = CHRONOS_DEBUG_LEVEL_MIN;

#define set_chronos_debug_level(_level)  \
  (chronos_debug_level = (_level))

#define chronos_debug(level,...) \
  do {                                                         \
    if (chronos_debug_level >= level) {                        \
      char _local_buf_[256];                                   \
      snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
      printf("%s:%d: %s", __FILE__, __LINE__, _local_buf_);    \
      printf("\n");	   \
    } \
  } while(0)



/*---------------------------------
 * Error routines
 *-------------------------------*/
#define chronos_msg(_prefix, ...) \
  do {                     \
    char _local_buf_[256];				     \
    snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
    printf("%s: %s: at %s:%d", _prefix,_local_buf_, __FILE__, __LINE__);   \
    printf("\n");					     \
  } while(0)

#define chronos_info(...) \
  chronos_msg("INFO", __VA_ARGS__)

#define chronos_error(...) \
  chronos_msg("ERROR", __VA_ARGS__)

#define chronos_warning(...) \
  chronos_msg("WARN", __VA_ARGS__)
