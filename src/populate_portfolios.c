/*
 * =====================================================================================
 *
 *       Filename:  populate_portfolios.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/14/16 19:31:17
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricardo Zavaleta (rj.zavaleta@gmail.com)
 *   Organization:  Cinvestav
 *
 * =====================================================================================
 */
#include "benchmark.h"
#include "benchmark_common.h"
#include <time.h>

/*============================================================================
 *                          PROTOTYPES
 *============================================================================*/
static int
load_portfolio_database(BENCHMARK_DBS *benchmarkP);

int
benchmark_load_portfolio(void *benchmark_handle)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  int ret;


  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }
  
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  ret = load_portfolio_database(benchmarkP);
  if (ret) {
    benchmark_error("%s:%d Error loading personal database.", __FILE__, __LINE__);
    goto failXit;
  }
  BENCHMARK_CHECK_MAGIC(benchmarkP);

  benchmark_debug(1, "Done populating portfolios.");
  
  return BENCHMARK_SUCCESS;

failXit:
  return BENCHMARK_FAIL;
}

static int
load_portfolio_database(BENCHMARK_DBS *benchmarkP)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  PORTFOLIOS portfolio;
  int i;

  envP = benchmarkP->envP;
  if (envP == NULL || benchmarkP->portfolios_dbp == NULL) {
    benchmark_error("%s: Invalid arguments", __func__);
    goto failXit;
  }

  benchmark_debug(3,"LOADING PORTFOLIOS DATABASE ");

  for (i=0 ; i<CHRONOS_PORTFOLIOS_NUM; i++) {

    /* zero out the structure */
    memset(&portfolio, 0, sizeof(PORTFOLIOS));

    /* Zero out the DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    sprintf(portfolio.portfolio_id, "%d", i);
    sprintf(portfolio.account_id, "%d", (rand() % 50)+1);
    sprintf(portfolio.symbol, "%s", symbolsArr[(rand() % 10)]);
    portfolio.hold_stocks = (rand() % 100) + 1;

    portfolio.to_sell = portfolio.hold_stocks ? (rand()%2) : 0;
    portfolio.number_sell = portfolio.to_sell ? (rand() % portfolio.hold_stocks) + 1 : 0;
    portfolio.price_sell = portfolio.to_sell ? (rand() % 100) + 1 : 0;

    portfolio.to_buy = (rand()%2);
    portfolio.number_buy = portfolio.to_buy ? (rand() % 20) +1 : 0;
    portfolio.price_buy = portfolio.to_buy ? (rand() % 100) + 1 : 0;

    /* Now that we have our structure we can load it into the database. */

    /* Set up the database record's key */
    key.data = portfolio.portfolio_id;
    key.size = (u_int32_t)strlen(portfolio.portfolio_id) + 1;

    /* Set up the database record's data */
    data.data = &portfolio;
    data.size = sizeof(PORTFOLIOS);

    /*
     * Note that given the way we built our struct, there's extra
     * bytes in it. Essentially we're using fixed-width fields with
     * the unused portion of some fields padded with zeros. This
     * is the easiest thing to do, but it does result in a bloated
     * database. 
     */

    /* Put the data into the database */
    benchmark_debug(4,"Inserting: %s", (char *)key.data);

    rc = envP->txn_begin(envP, NULL, &txnP, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction begin failed.");
      goto failXit; 
    }

    rc = benchmarkP->portfolios_dbp->put(benchmarkP->portfolios_dbp, txnP, &key, &data, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Database put failed.");
      txnP->abort(txnP);
      goto failXit; 
    }

    rc = txnP->commit(txnP, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction commit failed.");
      goto failXit; 
    }
  }

  return BENCHMARK_SUCCESS;

failXit:
  return BENCHMARK_FAIL;
}
