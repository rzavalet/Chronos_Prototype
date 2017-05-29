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

int 
benchmark_handle_alloc(void **benchmark_handle, int create, char *homedir, char *datafilesdir)
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
    symbol = rand() % BENCHMARK_NUM_SYMBOLS;
  } 

  random_symbol = symbolsArr[symbol];
#if 0
  ret = show_stocks_records(random_symbol, benchmarkP);
#endif
  ret = show_quote(random_symbol, benchmarkP);

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
