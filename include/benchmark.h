#ifndef _BENCHMARK_H_
#define _BENCHMARK_H_

#include "benchmark_common.h"

typedef void *BENCHMARK_H;

int 
benchmark_handle_alloc(void **benchmark_handle, 
                       int create, 
                       const char *program, 
                       const char *homedir, 
                       const char *datafilesdir);

int 
benchmark_handle_free(BENCHMARK_H benchmark_handle);

int 
benchmark_initial_load(const char *program,
                       const char *homedir, 
                       const char *datafilesdir);

int
benchmark_load_portfolio(BENCHMARK_H benchmark_handle);

int
benchmark_refresh_quotes(BENCHMARK_H benchmark_handle, 
                         int *symbolP, 
                         float newValue);

int
benchmark_refresh_quotes2(BENCHMARK_H benchmark_handle, 
                          const char *symbolP, 
                          float newValue);

int
benchmark_view_stock(BENCHMARK_H benchmark_handle, 
                     int *symbolP);

int
benchmark_view_stock2(int num_symbols, 
                      const char **symbol_list_P, 
                      BENCHMARK_H benchmark_handle);

int
benchmark_portfolios_stats_get(BENCHMARK_DBS *benchmarkP);

int
benchmark_view_portfolio(BENCHMARK_H  benchmark_handle);

int
benchmark_purchase2(int                   num_data,
                    benchmark_xact_data_t *data,
                    BENCHMARK_H           benchmark_handle);
int
benchmark_view_portfolio2(int           num_accounts, 
                          const char    **account_list_P, 
                          BENCHMARK_H   benchmark_handle);
int
benchmark_purchase(int      account, 
                   int      symbol, 
                   float    price, 
                   int      amount, 
                   int      force_apply, 
                   BENCHMARK_H  benchmark_handle, 
                   int      *symbolP);

int
benchmark_sell(int    account, 
               int    symbol, 
               float  price, 
               int    amount, 
               int    force_apply, 
               BENCHMARK_H benchmark_handle, 
               int    *symbol_ret);

int
benchmark_sell2(int           num_data,
                benchmark_xact_data_t *data,
                void *benchmark_handle);

int
benchmark_stock_list_get(BENCHMARK_H benchmark_handle, 
                         char ***stocks_list, 
                         int *num_stocks);


#endif
