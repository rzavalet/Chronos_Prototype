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
benchmark_view_portfolio(void *benchmark_handle)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  ret = show_portfolios(NULL, 0, NULL, benchmarkP);
  BENCHMARK_CHECK_MAGIC(benchmarkP);

  return (ret);

 failXit:
  return BENCHMARK_FAIL;
}

int
benchmark_view_portfolio2(int           num_accounts, 
                          const char    **account_list_P, 
                          void          *benchmark_handle)
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

  benchmark_debug(2, "Showing portfolio for: %d users", num_accounts);

  for (i=0; i<num_accounts; i++) {
    benchmark_debug(2, "Showing quote for user: %s", account_list_P[i]);
    ret = show_portfolios((char *)account_list_P[i], 0, xactH, benchmarkP);
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
