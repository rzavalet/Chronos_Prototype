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
#include "benchmark_common.h"
#include <time.h>

/* Forward declarations */
int 
usage(void);

int
load_portfolio_database(BENCHMARK_DBS my_benchmarkP);

int
usage()
{
  fprintf(stderr, "populate_portfolios\n");
  fprintf(stderr, "\t-h <database_home_directory>\n");

  fprintf(stderr, "\tNote: Any path specified must end with your");
  fprintf(stderr, " system's path delimiter (/ or \\)\n");
  return (-1);
}

int
main(int argc, char *argv[])
{
  BENCHMARK_DBS my_benchmark;
  int ch, ret;
  size_t size;
  char *basename = NULL, *personal_file, *stocks_file, *currencies_file;

  initialize_benchmarkdbs(&my_benchmark);

  basename = "./";

  /* Parse the command line arguments */
  while ((ch = getopt(argc, argv, "b:h:")) != EOF) {
    switch (ch) {
      case 'h':
        if (optarg[strlen(optarg)-1] != '/' &&
            optarg[strlen(optarg)-1] != '\\')
        return (usage());

        my_benchmark.db_home_dir = optarg;
        break;

      case '?':

      default:
          return (usage());
    }
  }

  if (my_benchmark.db_home_dir == NULL) {
    fprintf(stderr, "You must specify -h\n");
    return usage();
  }

  set_db_filenames(&my_benchmark);

  ret = databases_setup(&my_benchmark, ALL_DBS_FLAG, __FILE__, stderr);
  if (ret) {
    fprintf(stderr, "%s:%d Error opening databases.\n", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_portfolio_database(my_benchmark);
  if (ret) {
    fprintf(stderr, "%s:%d Error loading personal database.\n", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  databases_close(&my_benchmark);

  printf("Done loading databases.\n");
  return (ret);
}

int
load_portfolio_database(BENCHMARK_DBS my_benchmarkP)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  PORTFOLIOS portfolio;
  char *symbolsArr[10] = {"YHOO", "AAPL", "GOOG", "MSFT", "PIH", "FLWS", "SRCE", "VNET", "TWOU", "JOBS"};
  int i;

  envP = my_benchmarkP.envP;
  if (envP == NULL || my_benchmarkP.portfolios_dbp == NULL) {
    fprintf(stderr, "%s: Invalid arguments\n", __func__);
    goto failXit;
  }

  srand(time(NULL));

  printf("================= LOADING PORTFOLIOS DATABASE ==============\n");

  for (i=0 ; i<CHRONOS_PORTFOLIOS_NUM; i++) {

    /* zero out the structure */
    memset(&portfolio, 0, sizeof(PORTFOLIOS));

    /* Zero out the DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    sprintf(portfolio.portfolio_id, "%d", i);
    sprintf(portfolio.account_id, "%d", (rand() % 50)+1);
    sprintf(portfolio.symbol, "%s", symbolsArr[(rand() % 10)]);
    portfolio.hold_stocks = (rand() % 100);
    portfolio.to_sell = portfolio.hold_stocks ? (rand()%2) : 0;
    portfolio.number_sell = portfolio.to_sell ? (rand() % portfolio.hold_stocks) : 0;
    portfolio.price_sell = (rand() % 100);
    portfolio.to_buy = (rand()%2);
    portfolio.number_buy = portfolio.to_buy ? (rand() % 20) : 0;
    portfolio.price_buy = (rand() % 100);

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
    printf("Inserting: %s\n", (char *)key.data);

    rc = envP->txn_begin(envP, NULL, &txnP, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction begin failed.");
      goto failXit; 
    }

    rc = my_benchmarkP.portfolios_dbp->put(my_benchmarkP.portfolios_dbp, txnP, &key, &data, 0);
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

  return (0);

failXit:
  return 1;
}
