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
benchmark_handle_alloc(void **benchmark_handle, char *homedir)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret = BENCHMARK_FAIL;
  
  benchmarkP = malloc(sizeof (BENCHMARK_DBS));
  if (benchmarkP == NULL) {
    goto failXit;
  }

  memset(benchmarkP, 0, sizeof(*benchmarkP));
  
  initialize_benchmarkdbs(benchmarkP);
  benchmarkP->db_home_dir = homedir;
  
  /* Identify the files that hold our databases */
  set_db_filenames(benchmarkP);

  ret = databases_setup(benchmarkP, ALL_DBS_FLAG, __FILE__, stderr);
  if (ret != 0) {
    fprintf(stderr, "Error opening databases\n");
    databases_close(benchmarkP);
    goto failXit;
  }

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

  databases_close(benchmarkP);
  free(benchmarkP);
  
  return BENCHMARK_SUCCESS;
}

int
benchmark_view_stock(void *benchmark_handle)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }
  
  ret = show_stocks_records(NULL, benchmarkP);

  return ret;

 failXit:
  return BENCHMARK_FAIL;
}
