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

/*============================================================================
 *                          PROTOTYPES
 *============================================================================*/
static int
load_currencies_database(BENCHMARK_DBS my_benchmarkP, char *currencies_file);

static int
load_stocks_database(BENCHMARK_DBS my_benchmarkP, char *stocks_file);

static int
load_personal_database(BENCHMARK_DBS my_benchmarkP, char *personal_file);

int 
benchmark_initial_load(char *homedir, char *datafilesdir) 
{
  BENCHMARK_DBS my_benchmark;
  char *personal_file, *stocks_file, *currencies_file;
  int size;
  int ret;

  initialize_benchmarkdbs(&my_benchmark);
  my_benchmark.db_home_dir = homedir;
  set_db_filenames(&my_benchmark);

  /* Find our input files */
  size = strlen(datafilesdir) + strlen(PERSONAL_FILE) + 2;
  personal_file = malloc(size);
  snprintf(personal_file, size, "%s/%s", datafilesdir, PERSONAL_FILE);

  size = strlen(datafilesdir) + strlen(STOCKS_FILE) + 2;
  stocks_file = malloc(size);
  snprintf(stocks_file, size, "%s/%s", datafilesdir, STOCKS_FILE);

  size = strlen(datafilesdir) + strlen(CURRENCIES_FILE) + 2;
  currencies_file = malloc(size);
  snprintf(currencies_file, size, "%s/%s", datafilesdir, CURRENCIES_FILE);

  ret = databases_setup(&my_benchmark, ALL_DBS_FLAG, "benchmark_initial_load", stderr);
  if (ret) {
    benchmark_error("%s:%d Error opening databases.", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_personal_database(my_benchmark, personal_file);
  if (ret) {
    benchmark_error("%s:%d Error loading personal database.", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_stocks_database(my_benchmark, stocks_file);
  if (ret) {
    benchmark_error("%s:%d Error loading stocks database.", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_currencies_database(my_benchmark, currencies_file);
  if (ret) {
    benchmark_error("%s:%d Error loading currencies database.", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  databases_close(&my_benchmark);

  benchmark_debug(1, "Done with initial load ...");

  return BENCHMARK_SUCCESS;
}


static int
load_personal_database(BENCHMARK_DBS my_benchmarkP, char *personal_file)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  PERSONAL my_personal;

  envP = my_benchmarkP.envP;
  if (envP == NULL || personal_file == NULL) {
    benchmark_error("%s: Invalid arguments", __func__);
    goto failXit;
  }

  benchmark_debug(3,"LOADING PERSONAL DATABASE ");

  ifp = fopen(personal_file, "r");
  if (ifp == NULL) {
    benchmark_error("Error opening file '%s'", personal_file);
    goto failXit;
  }

  /* Iterate over the vendor file */
  while (fgets(buf, MAXLINE, ifp) != NULL) {

    /* zero out the structure */
    memset(&my_personal, 0, sizeof(PERSONAL));

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
      "%10[^#]#%128[^#]#%128[^#]#%128[^#]#%128[^#]#%128[^#]#%128[^#]#%16[^\n]",
      my_personal.account_id, my_personal.last_name,
      my_personal.first_name, my_personal.address,
      my_personal.city,
      my_personal.state, my_personal.country,
      my_personal.phone);

    /* Now that we have our structure we can load it into the database. */

    /* Set up the database record's key */
    key.data = my_personal.account_id;
    key.size = (u_int32_t)strlen(my_personal.account_id) + 1;

    /* Set up the database record's data */
    data.data = &my_personal;
    data.size = sizeof(PERSONAL);

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

    rc = my_benchmarkP.personal_dbp->put(my_benchmarkP.personal_dbp, txnP, &key, &data, 0);
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

static int
load_stocks_database(BENCHMARK_DBS my_benchmarkP, char *stocks_file)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  STOCK my_stocks;

  envP = my_benchmarkP.envP;
  if (envP == NULL || stocks_file == NULL) {
    benchmark_error("%s: Invalid arguments", __func__);
    goto failXit;
  }

  benchmark_debug(3,"LOADING STOCKS DATABASE ");

  ifp = fopen(stocks_file, "r");
  if (ifp == NULL) {
    benchmark_error("Error opening file '%s'", stocks_file);
    goto failXit;
  }

  /* Iterate over the vendor file */
  while (fgets(buf, MAXLINE, ifp) != NULL) {

    /* zero out the structure */
    memset(&my_stocks, 0, sizeof(STOCK));

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
      "%10[^#]#%128[^#]#%500[^\n]",
      my_stocks.stock_symbol, my_stocks.full_name, ignore_buf);

    /* Now that we have our structure we can load it into the database. */

    /* Set up the database record's key */
    key.data = my_stocks.stock_symbol;
    key.size = (u_int32_t)strlen(my_stocks.stock_symbol) + 1;

    /* Set up the database record's data */
    data.data = &my_stocks;
    data.size = sizeof(STOCK);

    /*
     * Note that given the way we built our struct, there's extra
     * bytes in it. Essentially we're using fixed-width fields with
     * the unused portion of some fields padded with zeros. This
     * is the easiest thing to do, but it does result in a bloated
     * database.
     */ 

    /* Put the data into the database */
    benchmark_debug(4,"Inserting: %s", (char *)key.data);
    benchmark_debug(4, "\t(%s, %s)", my_stocks.stock_symbol, my_stocks.full_name);

    rc = envP->txn_begin(envP, NULL, &txnP, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction begin failed.");
      goto failXit; 
    }

    rc = my_benchmarkP.stocks_dbp->put(my_benchmarkP.stocks_dbp, txnP, &key, &data, 0);

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

static int
load_currencies_database(BENCHMARK_DBS my_benchmarkP, char *currencies_file)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  CURRENCY my_currencies;
 
  envP = my_benchmarkP.envP;
  if (envP == NULL || currencies_file == NULL) {
    benchmark_error("%s: Invalid arguments", __func__);
    goto failXit;
  }

  benchmark_debug(3,"LOADING CURRENCIES DATABASE ");

  ifp = fopen(currencies_file, "r");
  if (ifp == NULL) {
    benchmark_error("Error opening file '%s'", currencies_file);
    goto failXit;
  }

  /* Iterate over the vendor file */
  while (fgets(buf, MAXLINE, ifp) != NULL) {

    /* zero out the structure */
    memset(&my_currencies, 0, sizeof(CURRENCY));

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
      "%200[^#]#%200[^#]#%10[^\n]",
      my_currencies.country, my_currencies.currency_name, my_currencies.currency_symbol);

    /* Now that we have our structure we can load it into the database. */

    /* Set up the database record's key */
    key.data = my_currencies.currency_symbol;
    key.size = (u_int32_t)strlen(my_currencies.currency_symbol) + 1;

    /* Set up the database record's data */
    data.data = &my_currencies;
    data.size = sizeof(CURRENCY);

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

    rc = my_benchmarkP.currencies_dbp->put(my_benchmarkP.currencies_dbp, txnP, &key, &data, 0);

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
