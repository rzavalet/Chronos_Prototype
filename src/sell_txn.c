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
    symbol_idx = rand() % benchmarkP->number_stocks;
    random_symbol = benchmarkP->stocks[symbol_idx];
  }
  else {
    random_symbol = benchmarkP->stocks[symbol];
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
 
  assert("Need to pass a valid account" == NULL);
  ret = sell_stocks(NULL, random_symbol, random_price, random_amount, force_apply, NULL, benchmarkP);
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

int
benchmark_sell2(int           num_data,
                benchmark_xact_data_t *data,
                void *benchmark_handle)
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

  benchmark_debug(2, "Sell for: %d symbols", num_data);

  for (i=0; i<num_data; i++) {
    benchmark_debug(2, "Placing order for user: %s", data[i].accountId);
    ret = sell_stocks(data[i].accountId, data[i].symbol, data[i].price, data[i].amount, 1, xactH, benchmarkP);
    if (ret != BENCHMARK_SUCCESS) {
      benchmark_error("Could not place order");
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
