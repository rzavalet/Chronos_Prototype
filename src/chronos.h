/*===================================
 * This is the MAIN chronos header file
 * Include this one in any file where
 * you want to call the chronos API
 *==================================*/

#if 0
#define CHRONOS_DEBUG
#endif

/*---------------------------------
 * Important CONSTANTS 
 *-------------------------------*/
#define CHRONOS_SUCCESS         0
#define CHRONOS_FAIL            1

#define chronos_debug(...) \
  do {                     \
    char _local_buf_[256]; \
    snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
    printf("%s:%d: %s", __FILE__, __LINE__, _local_buf_);    \
    printf("\n");	   \
  } while(0)

#define chronos_error(...) \
  do {                     \
    char _local_buf_[256];				     \
    snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
    printf("ERROR: %s: at %s:%d", _local_buf_, __FILE__, __LINE__);   \
    printf("\n");					     \
  } while(0)

#define chronos_warning(...) \
  do {                     \
    char _local_buf_[256];				     \
    snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
    printf("WARN: %s: at %s:%d", _local_buf_, __FILE__, __LINE__);    \
    printf("\n");					     \
  } while(0)
