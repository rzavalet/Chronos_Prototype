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
      fprintf(stderr, "%s:%d: %s", __FILE__, __LINE__, _local_buf_);    \
      fprintf(stderr, "\n");	   \
    } \
  } while(0)



/*---------------------------------
 * Error routines
 *-------------------------------*/
#define chronos_msg(_prefix, ...) \
  do {                     \
    char _local_buf_[256];				     \
    snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
    fprintf(stderr,"%s: %s: at %s:%d", _prefix,_local_buf_, __FILE__, __LINE__);   \
    fprintf(stderr,"\n");					     \
  } while(0)

#define chronos_info(...) \
  chronos_msg("INFO", __VA_ARGS__)

#define chronos_error(...) \
  chronos_msg("ERROR", __VA_ARGS__)

#define chronos_warning(...) \
  chronos_msg("WARN", __VA_ARGS__)

typedef struct timespec chronos_time_t;
#define NSEC_TO_SEC (1000000000)
#define CHRONOS_TIME_FMT "%ld.%09ld"
#define CHRONOS_TIME_ARG(_t) (_t).tv_sec, (_t).tv_nsec
#define CHRONOS_TIME_CLEAR(_t) \
  do {\
    (_t).tv_sec = 0; \
    (_t).tv_nsec = 0; \
  }while (0)

#define CHRONOS_TIME_GET(_t) clock_gettime(CLOCK_REALTIME, &(_t))

#define CHRONOS_TIME_NANO_OFFSET_GET(_begin, _end, _off)		\
  do {									\
    chronos_time_t _tf = _end;						\
    chronos_time_t _to = _begin;					\
									\
    if (_tf.tv_sec > _to.tv_sec && _tf.tv_nsec < _to.tv_nsec) {		\
      _tf.tv_sec --;							\
      _tf.tv_nsec += NSEC_TO_SEC;					\
    }									\
    else if (_tf.tv_sec < _to.tv_sec && _tf.tv_nsec > _to.tv_nsec) {	\
      _to.tv_sec --;							\
      _to.tv_nsec += NSEC_TO_SEC;					\
    }									\
    (_off).tv_sec = _tf.tv_sec - _to.tv_sec;				\
    (_off).tv_nsec = _tf.tv_nsec - _to.tv_nsec;				\
  } while(0)

#define CHRONOS_TIME_SEC_OFFSET_GET(_begin, _end, _off)			\
  do {									\
    chronos_time_t _tf = _end;						\
    chronos_time_t _to = _begin;					\
									\
    if (_tf.tv_sec > _to.tv_sec && _tf.tv_nsec < _to.tv_nsec) {		\
      _tf.tv_sec --;							\
      _tf.tv_nsec += NSEC_TO_SEC;					\
    }									\
    else if (_tf.tv_sec < _to.tv_sec && _tf.tv_nsec > _to.tv_nsec) {	\
      _to.tv_sec --;							\
      _to.tv_nsec += NSEC_TO_SEC;					\
    }									\
    (_off).tv_sec = _tf.tv_sec - _to.tv_sec;				\
    (_off).tv_nsec = 0;							\
  } while(0)

#define CHRONOS_TIME_ADD(_t1, _t2, _res)			\
      do {							\
	chronos_time_t _tf = _t1;				\
	chronos_time_t _to = _t2;				\
	(_res).tv_sec = 0;					\
	(_res).tv_nsec = (_tf).tv_nsec + (_to).tv_nsec;		\
								\
	if ((_res).tv_nsec > NSEC_TO_SEC) {			\
	  (_res).tv_sec = (_res).tv_nsec / NSEC_TO_SEC;	\
	  (_res).tv_nsec = (_res).tv_nsec % NSEC_TO_SEC;	\
	}							\
								\
	(_res).tv_sec += (_tf).tv_sec + (_to).tv_sec;		\
								\
      } while(0)
      
#define CHRONOS_TIME_AVERAGE(_sum, _num, _res)				\
  do {									\
    if (_num != 0) {							\
      long long mod;							\
      (_res).tv_sec  = (_sum).tv_sec / (signed long)_num;		\
      mod =  NSEC_TO_SEC * ((_sum).tv_sec % (signed long)_num);		\
      (_res).tv_nsec = (mod + (_sum).tv_nsec) / (signed long)_num;	\
      if ((_res).tv_nsec >= NSEC_TO_SEC) {				\
	mod = (_res).tv_nsec / NSEC_TO_SEC;				\
	(_res).tv_sec += mod;						\
	mod = (_res).tv_nsec % NSEC_TO_SEC;				\
	(_res).tv_nsec = mod;						\
      }									\
    }									\
  }while(0)

#define CHRONOS_TIME_POSITIVE(_time)		\
  ((_time).tv_sec > 0 && (_time).tv_nsec > 0)

#define CHRONOS_TIME_ZERO(_time)		\
  ((_time).tv_sec == 0 && (_time).tv_nsec == 0)

#define CHRONOS_TIME_NEGATIVE(_time)		\
  ((_time).tv_sec < 0 && (_time).tv_nsec < 0)

#define SEC_TO_MSEC   (1000)
#define NSEC_TO_MSEC  (1000000)

#define CHRONOS_TIME_TO_MS(_time) \
  ((_time).tv_sec * SEC_TO_MSEC + (_time).tv_nsec / NSEC_TO_MSEC)

#define CHRONOS_MS_TO_TIME(_ms, _time) \
  do { \
    (_time).tv_sec = (_ms) / SEC_TO_MSEC;	\
    (_time).tv_nsec = ((_ms) % SEC_TO_MSEC) * NSEC_TO_MSEC; \
  } while (0) 
#endif
