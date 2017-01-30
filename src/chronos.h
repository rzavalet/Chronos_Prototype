/*===================================
 * This is the MAIN chronos header file
 * Include this one in any file where
 * you want to call the chronos API
 *==================================*/
#ifndef _CHRONOS_H_
#define _CHRONOS_H_


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

typedef enum chronos_system_transaction_t {
  CHRONOS_SYS_TXN_MIN = 0,
  CHRONOS_SYS_TXN_UPDATE_STOCK = CHRONOS_SYS_TXN_MIN,
  CHRONOS_SYS_TXN_MAX,
  CHRONOS_SYS_TXN_INVAL=CHRONOS_SYS_TXN_MAX
} chronos_system_transaction_t;

const char *chronos_user_transaction_str[] = {
  "CHRONOS_USER_TXN_VIEW_STOCK",
  "CHRONOS_USER_TXN_VIEW_PORTFOLIO",
  "CHRONOS_USER_TXN_PURCHASE",
  "CHRONOS_USER_TXN_SALE"
};

const char *chronos_system_transaction_str[] = {
  "CHRONOS_SYS_TXN_UPDATE_STOCK"
};

#define CHRONOS_TXN_NAME(_txn_type) \
  ((CHRONOS_USER_TXN_MIN<=(_txn_type) && (_txn_type) < CHRONOS_USER_TXN_MAX) ? (chronos_user_transaction_str[(_txn_type)]) : "INVALID")

#define CHRONOS_TXN_IS_VALID(_txn_type) \
  (CHRONOS_USER_TXN_MIN<=(_txn_type) && (_txn_type) < CHRONOS_USER_TXN_MAX)


/*---------------------------------
 * Debugging routines
 *-------------------------------*/
#define CHRONOS_DEBUG_LEVEL_MIN   (0)
#define CHRONOS_DEBUG_LEVEL_MAX   (10)

extern int chronos_debug_level;

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

typedef struct timespec chronos_time_t;
#define CHRONOS_TIME_FMT "%ld.%09ld"
#define CHRONOS_TIME_ARG(_t) (_t).tv_sec, (_t).tv_nsec
#define CHRONOS_TIME_CLEAR(_t) \
  do {\
    (_t).tv_sec = 0; \
    (_t).tv_nsec = 0; \
  }while (0)

#define CHRONOS_TIME_GET(_t) clock_gettime(CLOCK_REALTIME, &(_t))

#if 1
#define CHRONOS_TIME_NANO_OFFSET_GET(_begin, _end, _off)	\
  do {								\
    chronos_time_t _tf = _end;					\
    chronos_time_t _to = _begin;				\
    if (_to.tv_nsec > _tf.tv_nsec) {				\
      _tf.tv_nsec += 1000000000;				\
      _tf.tv_sec --;						\
    }								\
								\
    (_off).tv_sec = _tf.tv_sec - _to.tv_sec;		\
    (_off).tv_nsec = _tf.tv_nsec - _to.tv_nsec;		\
								\
  } while(0)
#else
#define CHRONOS_TIME_NANO_OFFSET_GET(_begin, _end, _off)		\
  do {									\
    long long _tf = (_end).tv_sec * 1000000000 + (_end).tv_nsec;	\
    long long _to = (_begin).tv_sec * 1000000000 + (_begin).tv_nsec;	\
    long long _res = _tf - _to;						\
    (_off).tv_sec = _res / 1000000000;					\
    (_off).tv_nsec = _res % 1000000000;					\
  } while(0)
#endif

#define CHRONOS_TIME_SEC_OFFSET_GET(_begin, _end, _off)		\
  do {								\
    chronos_time_t _tf = _end;					\
    chronos_time_t _to = _begin;				\
    if (_to.tv_nsec > _tf.tv_nsec) {				\
      _tf.tv_nsec += 1000000000;				\
      _tf.tv_sec --;						\
    }								\
								\
    _off = _tf.tv_sec - _to.tv_sec;				\
  } while(0)

#define CHRONOS_TIME_ADD(_t1, _t2, _res)			\
      do {							\
	chronos_time_t _tf = _t1;				\
	chronos_time_t _to = _t2;				\
	(_res).tv_nsec = (_tf).tv_nsec + (_to).tv_nsec;		\
								\
	if ((_res).tv_nsec > 1000000000) {			\
	  long long int tmp = (_res).tv_nsec / 1000000000;	\
	  (_res).tv_nsec = (_res).tv_nsec % 1000000000;		\
	  (_res).tv_sec = tmp;					\
	}							\
								\
	(_res).tv_sec += (_tf).tv_sec + (_to).tv_sec;		\
								\
      } while(0)
      
#define CHRONOS_TIME_AVERAGE(_sum, _num, _res)				\
  do {									\
    if (_num != 0) {							\
      long long mod;								\
      (_res).tv_sec  = (_sum).tv_sec / _num;				\
      mod =  1000000000 * (((float)(_sum).tv_sec / (float)_num) - (_res).tv_sec);	\
      (_res).tv_nsec = mod + (_sum).tv_nsec / _num;			\
      if ((_res).tv_nsec > 999999999) {					\
	mod = (_res).tv_nsec / 1000000000;					\
	(_res).tv_sec += mod;						\
	mod = (_res).tv_nsec % 1000000000;					\
	(_res).tv_nsec = mod;						\
      }									\
    }									\
  }while(0)

#define CHRONOS_TIME_POSITIVE(_time)		\
  ((_time).tv_sec >= 0 && (_time).tv_nsec >= 0)


#endif
