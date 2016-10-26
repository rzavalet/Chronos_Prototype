#ifndef _BENCHMARK_H_
#define _BENCHMARK_H_

#define BENCHMARK_SUCCESS   (0)
#define BENCHMARK_FAIL      (1)

int 
benchmark_initial_load(char *homedir, char *datafilesdir);

int
benchmark_load_portfolio(char *homedir);

int
benchmark_refresh_quotes(char *homedir, char *datafilesdir);

int
benchmark_view_stock(char *homedir);

int
benchmark_view_portfolio(char *homedir);

int
benchmark_purchase(char *homedir);

int
benchmark_sell(char *homedir);


/*---------------------------------
 * Debugging routines
 *-------------------------------*/
#define BENCHMARK_DEBUG_LEVEL_MIN   (0)
#define BENCHMARK_DEBUG_LEVEL_MAX   (10)

extern int benchmark_debug_level;

#define set_benchmark_debug_level(_level)  \
  (benchmark_debug_level = (_level))

#define benchmark_debug(level,...) \
  do {                                                         \
    if (benchmark_debug_level >= level) {                        \
      char _local_buf_[256];                                   \
      snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
      printf("%s:%d: %s", __FILE__, __LINE__, _local_buf_);    \
      printf("\n");	   \
    } \
  } while(0)



/*---------------------------------
 * Error routines
 *-------------------------------*/
#define benchmark_msg(_prefix, _fp, ...) \
  do {                     \
    char _local_buf_[256];				     \
    snprintf(_local_buf_, sizeof(_local_buf_), __VA_ARGS__); \
    fprintf(_fp, "%s: %s: at %s:%d", _prefix,_local_buf_, __FILE__, __LINE__);   \
    fprintf(_fp,"\n");					     \
  } while(0)

#define benchmark_info(...) \
  benchmark_msg("INFO", stdout, __VA_ARGS__)

#define benchmark_error(...) \
  benchmark_msg("ERROR", stderr, __VA_ARGS__)

#define benchmark_warning(...) \
  benchmark_msg("WARN", stdout, __VA_ARGS__)

#endif
