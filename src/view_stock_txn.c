/*
 * =====================================================================================
 *
 *       Filename:  view_stock_txn.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/16/16 23:33:23
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricardo Zavaleta (), rj.zavaleta@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "benchmark.h"
#include "benchmark_common.h"
#include "benchmark_stocks.h"

int 
benchmark_handle_alloc(void **benchmark_handle, int create, const char *homedir, const char *datafilesdir)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret = BENCHMARK_FAIL;
  
  benchmarkP = malloc(sizeof (BENCHMARK_DBS));
  if (benchmarkP == NULL) {
    goto failXit;
  }

  initialize_benchmarkdbs(benchmarkP);
  benchmarkP->magic = BENCHMARK_MAGIC_WORD;
  if (create) benchmarkP->createDBs = 1;
  benchmarkP->db_home_dir = homedir;
  benchmarkP->datafilesdir = datafilesdir;
  
  /* Identify the files that hold our databases */
  set_db_filenames(benchmarkP);

  ret = databases_setup(benchmarkP, ALL_DBS_FLAG, __FILE__, stderr);
  if (ret != 0) {
    benchmark_error("Error opening databases.");
    databases_close(benchmarkP);
    goto failXit;
  }

  if (!create) {
#if 0
    if (benchmark_stocks_stats_get(benchmarkP) != BENCHMARK_SUCCESS) {
      benchmark_error("Could not obtain stats for Stocks table.");
      databases_close(benchmarkP);
      goto failXit;
    }
#endif
    if (benchmark_stocks_symbols_get(benchmarkP) != BENCHMARK_SUCCESS) {
      benchmark_error("Could not obtain list of stock symbols.");
      databases_close(benchmarkP);
      goto failXit;
    }

#ifdef BENCHMARK_DEBUG
    if (benchmark_stocks_symbols_print(benchmarkP) != BENCHMARK_SUCCESS) {
      benchmark_error("Could not print stock symbols.");
      databases_close(benchmarkP);
      goto failXit;
    }
#endif

  }

  if (create) benchmarkP->createDBs = 0;
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  
  *benchmark_handle = benchmarkP;
  
  return BENCHMARK_SUCCESS;
  
 failXit:
  *benchmark_handle = NULL;
  return BENCHMARK_FAIL;
}

int 
benchmark_handle_free(void *benchmark_handle)
{
  BENCHMARK_DBS *benchmarkP = benchmark_handle;

  if (benchmarkP == NULL) {
    return BENCHMARK_SUCCESS;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  if (databases_close(benchmarkP) != BENCHMARK_SUCCESS) {
    return BENCHMARK_FAIL;
  }
  if (close_environment(benchmarkP) != BENCHMARK_SUCCESS) {
    return BENCHMARK_FAIL;
  }

  free(benchmarkP->stocks_db_name);
  free(benchmarkP->quotes_db_name);
  free(benchmarkP->quotes_hist_db_name);
  free(benchmarkP->portfolios_db_name);
  free(benchmarkP->portfolios_sdb_name);
  free(benchmarkP->accounts_db_name);
  free(benchmarkP->currencies_db_name);
  free(benchmarkP->personal_db_name);

  /* Don't forget to free the list of stocks */
  if (benchmarkP->number_stocks > 0 && benchmarkP->stocks != NULL) {
    int i;
    for (i=0; i<benchmarkP->number_stocks; i++) {
      if (benchmarkP->stocks[i] != NULL) {
        free(benchmarkP->stocks[i]);
        benchmarkP->stocks[i] = NULL;
      }
    }
  }

  benchmarkP->magic = 0;
  free(benchmarkP);
  
  return BENCHMARK_SUCCESS;
}

int
benchmark_view_stock(void *benchmark_handle, int *symbolP)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;
  int symbol;
  char *random_symbol;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }
  
  BENCHMARK_CHECK_MAGIC(benchmarkP);

  if (symbolP != NULL && *symbolP >= 0) {
    symbol = *symbolP;
  }
  else {
    symbol = rand() % benchmarkP->number_stocks;
  } 

  random_symbol = benchmarkP->stocks[symbol];
#if 0
  ret = show_stocks_records(random_symbol, benchmarkP);
#endif
  ret = show_quote(random_symbol, NULL, benchmarkP);

  if (symbolP != NULL) {
    *symbolP = symbol;
  }
    
  BENCHMARK_CHECK_MAGIC(benchmarkP);

  return ret;

 failXit:
  if (symbolP != NULL) {
    *symbolP = -1;
  }  
  
  return BENCHMARK_FAIL;
}

int
benchmark_view_stock2(int num_symbols, const char **symbol_list_P, void *benchmark_handle)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  benchmark_xact_h xactH = NULL;
  int i;
  int ret;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }
  
  BENCHMARK_CHECK_MAGIC(benchmarkP);

  ret = start_xact(&xactH, benchmarkP);
  if (ret != BENCHMARK_SUCCESS) {
    goto failXit;
  }

  benchmark_debug(2, "Showing quotes for: %d symbols", num_symbols);

  for (i=0; i<num_symbols; i++) {
    benchmark_debug(2, "Showing quote for symbol: %s", symbol_list_P[i]);
    ret = show_quote((char *)symbol_list_P[i], xactH, benchmarkP);
    if (ret != BENCHMARK_SUCCESS) {
      goto failXit;
    }
  }

  ret = commit_xact(xactH, benchmarkP);
  if (ret != BENCHMARK_SUCCESS) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  return ret;

 failXit:
  if (xactH != NULL) {
    abort_xact(xactH, benchmarkP);
  }

  return BENCHMARK_FAIL;
}
