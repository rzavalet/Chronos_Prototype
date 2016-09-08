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

/* Forward declarations */
int 
usage(void);

int
load_quotes_database(BENCHMARK_DBS my_benchmarkP, char *quotes_file);

int
usage()
{
  fprintf(stderr, "refresh_quotes\n");
  fprintf(stderr, "\t-b <path to data files>\n");
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
  char *basename = NULL, *quotes_file;

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

      case 'b':
        if (basename[strlen(basename)-1] != '/' &&
            basename[strlen(basename)-1] != '\\')
          return (usage());

        basename = optarg;
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

  if (basename == NULL) {
    fprintf(stderr, "You must specify -b\n");
    return usage();
  }

  set_db_filenames(&my_benchmark);

  /* Find our input files */
  size = strlen(basename) + strlen(QUOTES_FILE) + 1;
  quotes_file = malloc(size);
  snprintf(quotes_file, size, "%s%s", basename, QUOTES_FILE);

  ret = databases_setup(&my_benchmark, QUOTES_FLAG, "__FILE__", stderr);
  if (ret) {
    fprintf(stderr, "%s:%d Error opening databases.\n", __FILE__, __LINE__);
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_quotes_database(my_benchmark, quotes_file);
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
load_quotes_database(BENCHMARK_DBS my_benchmarkP, char *quotes_file)
{
  int rc = 0;
  DBT key, data;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  QUOTE quote;

  envP = my_benchmarkP.envP;
  if (envP == NULL || my_benchmarkP.quotes_dbp == NULL || quotes_file == NULL) {
    fprintf(stderr, "%s: Invalid arguments\n", __func__);
    goto failXit;
  }

  printf("================= LOADING QUOTES DATABASE ==============\n");

  ifp = fopen(quotes_file, "r");
  if (ifp == NULL) {
    fprintf(stderr, "Error opening file '%s'\n", quotes_file);
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
    printf("Inserting: %s\n", (char *)key.data);

    rc = envP->txn_begin(envP, NULL, &txnP, 0);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction begin failed.");
      goto failXit; 
    }

    rc = my_benchmarkP.quotes_dbp->put(my_benchmarkP.quotes_dbp, txnP, &key, &data, 0);
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
  return (0);

failXit:
  if (ifp != NULL) {
    fclose(ifp);
  }

  return 1;
}


