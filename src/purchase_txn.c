/*
 * =====================================================================================
 *
 *       Filename:  purchace_txn.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/16/16 23:33:23
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricardo Zavaleta (), rj.zavaleta@gmail.com
 *   Organization:  Cinvestav
 *
 * =====================================================================================
 */

#include "benchmark.h"
#include "benchmark_common.h"

int
benchmark_purchase(char *homedir)
{
  BENCHMARK_DBS my_benchmark;
  int ret;
  int random_account;
  char *random_symbol;
  float random_price;
  int random_amount;

  /* Initialize the BENCHMARK_DBS struct */
  initialize_benchmarkdbs(&my_benchmark);
  my_benchmark.db_home_dir = homedir;

  /* Identify the files that hold our databases */
  set_db_filenames(&my_benchmark);

  /* Open stocks database */
  /* TODO: Open just the required dbs */
  ret = databases_setup(&my_benchmark, ALL_DBS_FLAG, __FILE__, stderr);
  if (ret != 0) {
    fprintf(stderr, "Error opening databases\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  srand(time(NULL));
  random_account = (rand() % 50) + 1;
  random_symbol = symbolsArr[rand() % 10];
  random_price = rand() % 100 + 1;
  random_amount = rand() % 20 + 1;
 
  ret = place_order(random_account, random_symbol, random_price, random_amount, &my_benchmark);
  if (ret != 0) {
    fprintf(stderr, "Could not place order\n");
    databases_close(&my_benchmark);
    return (ret);
  }
 
  /* close our databases */
  databases_close(&my_benchmark);
  return ret;
}
