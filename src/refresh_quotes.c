/*
 * =====================================================================================
 *
 *       Filename:  benchmark_initial_load.c
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

static int
load_quotes_database(BENCHMARK_DBS *benchmarkP, char *quotes_file);

int
benchmark_refresh_quotes(void *benchmark_handle, int *symbol)
{
  BENCHMARK_DBS *benchmarkP = NULL;
  char *datafilesdir = NULL;
  int ret = BENCHMARK_SUCCESS;
  size_t size;
  char *quotes_file;

  benchmarkP = benchmark_handle;
  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
#if 0
  datafilesdir = benchmarkP->datafilesdir;
  
  /* Find our input files */
  size = strlen(datafilesdir) + strlen(QUOTES_FILE) + 2;
  quotes_file = malloc(size);
  snprintf(quotes_file, size, "%s/%s", datafilesdir, QUOTES_FILE);

  ret = load_quotes_database(benchmarkP, quotes_file);
  if (ret) {
    benchmark_error( "%s:%d Error loading personal database.", __FILE__, __LINE__);
    goto failXit;
  }
#endif
  rc = envP->txn_begin(envP, NULL, &txnP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "Transaction begin failed.");
    goto failXit; 
  }

  rc = benchmarkP->quotes_dbp->put(benchmarkP->quotes_dbp, txnP, &key, &data, 0);
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
  
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  benchmark_debug(2, "Done loading databases.");
  return ret;
  
 failXit:
  return BENCHMARK_FAIL;
}

static int
load_quotes_database(BENCHMARK_DBS *benchmarkP, char *quotes_file)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  QUOTE quote;

  envP = benchmarkP->envP;
  if (envP == NULL || benchmarkP->quotes_dbp == NULL || quotes_file == NULL) {
    benchmark_error( "%s: Invalid arguments", __func__);
    goto failXit;
  }

  benchmark_debug(2, "LOADING QUOTES DATABASE");

  ifp = fopen(quotes_file, "r");
  if (ifp == NULL) {
    benchmark_error("Error opening file '%s'", quotes_file);
    goto failXit;
  }

  /* Iterate over the vendor file */
  while (fgets(buf, MAXLINE, ifp) != NULL) {

    /* zero out the structure */
    memset(&quote, 0, sizeof(QUOTE));

    /* Zero out the DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /*
     * Scan the line into the structure.
     * Convenient, but not particularly safe.
     * In a real program, there would be a lot more
     * defensive code here.
     */
    sscanf(buf,
      "%10[^#]#%f#%10[^#]#%f#%f#%f#%f#%f#%ld#%10[^\n]",
      quote.symbol, &quote.current_price,
      quote.trade_time, &quote.low_price_day,
      &quote.high_price_day, &quote.perc_price_change, &quote.bidding_price,
      &quote.asking_price, &quote.trade_volume, quote.market_cap);

    /* Now that we have our structure we can load it into the database. */

    /* Set up the database record's key */
    key.data = quote.symbol;
    key.size = (u_int32_t)strlen(quote.symbol) + 1;

    /* Set up the database record's data */
    data.data = &quote;
    data.size = sizeof(QUOTE);

    /*
     * Note that given the way we built our struct, there's extra
     * bytes in it. Essentially we're using fixed-width fields with
     * the unused portion of some fields padded with zeros. This
     * is the easiest thing to do, but it does result in a bloated
     * database. 
     */

    /* Put the data into the database */
    benchmark_debug(6,"Inserting: %s", (char *)key.data);

    rc = envP->txn_begin(envP, NULL, &txnP, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction begin failed.");
      goto failXit; 
    }

    rc = benchmarkP->quotes_dbp->put(benchmarkP->quotes_dbp, txnP, &key, &data, 0);
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

  fclose(ifp);
  return BENCHMARK_SUCCESS;

failXit:
  if (ifp != NULL) {
    fclose(ifp);
  }

  return BENCHMARK_FAIL;
}


