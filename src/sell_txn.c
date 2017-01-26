/*
 * =====================================================================================
 *
 *       Filename:  sell_txn.c
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
benchmark_sell(void *benchmark_handle)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;
  int random_account;
  char *random_symbol;
  float random_price;
  int random_amount;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }

  srand(time(NULL));
  random_account = (rand() % 50) + 1;
  random_symbol = symbolsArr[rand() % 10];
  random_price = rand() % 100 + 1;
  random_amount = rand() % 20 + 1;
 
  ret = sell_stocks(random_account, random_symbol, random_price, random_amount, benchmarkP);
  if (ret != 0) {
    fprintf(stderr, "Could not place order\n");
    goto failXit;
  }
 
  return ret;
  
 failXit:
  return BENCHMARK_FAIL;
}
