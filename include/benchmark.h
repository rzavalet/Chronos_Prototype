#ifndef _BENCHMARK_H_
#define _BENCHMARK_H_

#define BENCHMARK_NUM_SYMBOLS  (10)
#define BENCHMARK_NUM_ACCOUNTS (50)

#define BENCHMARK_SUCCESS   (0)
#define BENCHMARK_FAIL      (1)

int 
benchmark_handle_alloc(void **benchmark_handle, int create, char *homedir, char *datafilesdir);

int 
benchmark_handle_free(void *benchmark_handle);

int 
benchmark_initial_load(char *homedir, char *datafilesdir);

int
benchmark_load_portfolio(void *benchmark_handle);

int
benchmark_refresh_quotes(void *benchmark_handle, int *symbolP, float newValue);

int
benchmark_refresh_quotes2(void *benchmark_handle, const char *symbolP, float newValue);

int
benchmark_view_stock(void *benchmark_handle, int *symbolP);

int
benchmark_view_stock2(void *benchmark_handle, const char *symbolP);

int
benchmark_view_portfolio(void *benchmark_handle);

int
benchmark_purchase(int account, int symbol, float price, int amount, int force_apply, void *benchmark_handle, int *symbolP);

int
benchmark_sell(int account, int symbol, float price, int amount, int force_apply, void *benchmark_handle, int *symbol_ret);

int
benchmark_stock_list_get(void *benchmark_handle, 
                         char ***stocks_list, 
                         int *num_stocks);

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
      fprintf(stderr, "%s:%d: %s", __FILE__, __LINE__, _local_buf_);    \
      fprintf(stderr, "\n");	   \
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

#if BENCHMARK_DEBUG
#define benchmark_info(...) \
  benchmark_msg("INFO", stderr, __VA_ARGS__)
#else
#define benchmark_info(...)
#endif

#define benchmark_error(...) \
  benchmark_msg("ERROR", stderr, __VA_ARGS__)

#define benchmark_warning(...) \
  benchmark_msg("WARN", stderr, __VA_ARGS__)

#endif
