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
benchmark_view_stock(char *homedir)
{
  BENCHMARK_DBS my_benchmark;
  int ret;

  /* Initialize the BENCHMARK_DBS struct */
  initialize_benchmarkdbs(&my_benchmark);
  my_benchmark.db_home_dir = homedir;
  
  /* Identify the files that hold our databases */
  set_db_filenames(&my_benchmark);

  /* Open stocks database */
  ret = databases_setup(&my_benchmark, STOCKS_FLAG, __FILE__, stderr);
  if (ret != 0) {
    fprintf(stderr, "Error opening databases\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = show_stocks_records(NULL, &my_benchmark);

  /* close our databases */
  databases_close(&my_benchmark);
  return (ret);
}
