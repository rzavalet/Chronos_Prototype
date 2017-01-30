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
  ret = show_portfolios(benchmarkP);
  BENCHMARK_CHECK_MAGIC(benchmarkP);

  return (ret);

 failXit:
  return BENCHMARK_FAIL;
}
