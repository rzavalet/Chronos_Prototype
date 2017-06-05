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
benchmark_sell(int account, int symbol, float price, int amount, int force_apply, void *benchmark_handle, int *symbol_ret)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;
  int random_account;
  int symbol_idx;
  char *random_symbol;
  float random_price;
  int random_amount;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  if (account < 0) {
    random_account = (rand() % 50) + 1;
  }
  else {
    random_account = account;
  }

  if (symbol < 0) {
    symbol_idx = rand() % BENCHMARK_NUM_SYMBOLS;
    random_symbol = symbolsArr[symbol_idx];
  }
  else {
    random_symbol = symbolsArr[symbol];
  }

  if (price < 0) {
    random_price = rand() % 100 + 1;
  }
  else {
    random_price = price;
  }

  if (amount < 0) {
    random_amount = rand() % 20 + 1;
  }
  else {
    random_amount = amount;
  }
 
  ret = sell_stocks(random_account, random_symbol, random_price, random_amount, force_apply, benchmarkP);
  if (ret != 0) {
    benchmark_error("Could not place order");
    goto failXit;
  }

  if (symbol_ret != NULL) {
    *symbol_ret = symbol;
  }
      
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  return ret;
  
 failXit:
  if (symbol_ret != NULL) {
    *symbol_ret = -1;
  }
      
  return BENCHMARK_FAIL;
}
