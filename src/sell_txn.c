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
benchmark_sell(void *benchmark_handle, int *symbolP)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;
  int random_account;
  int symbol;
  char *random_symbol;
  float random_price;
  int random_amount;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  random_account = (rand() % 50) + 1;
  symbol = rand() % BENCHMARK_NUM_SYMBOLS;
  random_symbol = symbolsArr[symbol];
  random_price = rand() % 100 + 1;
  random_amount = rand() % 20 + 1;
 
  ret = sell_stocks(random_account, random_symbol, random_price, random_amount, benchmarkP);
  if (ret != 0) {
    fprintf(stderr, "Could not place order\n");
    goto failXit;
  }

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
