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
load_currencies_database(BENCHMARK_DBS my_benchmarkP, char *currencies_file);

int
load_stocks_database(BENCHMARK_DBS my_benchmarkP, char *stocks_file);

int
load_personal_database(BENCHMARK_DBS my_benchmarkP, char *personal_file);

int
usage()
{
  fprintf(stderr, "benchmark_initial_load [-b <path to data files>]");
  fprintf(stderr, " [-h <database_home_directory>]\n");

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
  char *basename, *personal_file, *stocks_file, *currencies_file;

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

  set_db_filenames(&my_benchmark);

  /* Find our input files */
  size = strlen(basename) + strlen(PERSONAL_FILE) + 1;
  personal_file = malloc(size);
  snprintf(personal_file, size, "%s%s", basename, PERSONAL_FILE);

  size = strlen(basename) + strlen(STOCKS_FILE) + 1;
  stocks_file = malloc(size);
  snprintf(stocks_file, size, "%s%s", basename, STOCKS_FILE);

  size = strlen(basename) + strlen(CURRENCIES_FILE) + 1;
  currencies_file = malloc(size);
  snprintf(currencies_file, size, "%s%s", basename, CURRENCIES_FILE);

  ret = databases_setup(&my_benchmark, "benchmark_initial_load", stderr);
  if (ret) {
    fprintf(stderr, "Error opening databases.\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_personal_database(my_benchmark, personal_file);
  if (ret) {
    fprintf(stderr, "Error loading personal database.\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_stocks_database(my_benchmark, stocks_file);
  if (ret) {
    fprintf(stderr, "Error loading stocks database.\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  ret = load_currencies_database(my_benchmark, currencies_file);
  if (ret) {
    fprintf(stderr, "Error loading currencies database.\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  databases_close(&my_benchmark);

  printf("Done loading databases.\n");
  return (ret);
}

int
load_personal_database(BENCHMARK_DBS my_benchmarkP, char *personal_file)
{
  DBT key, data;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  PERSONAL my_personal;

  printf("================= LOADING PERSONAL DATABASE ==============\n");

  ifp = fopen(personal_file, "r");
  if (ifp == NULL) {
    fprintf(stderr, "Error opening file '%s'\n", personal_file);
    return (-1);
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
    printf("Inserting: %s\n", key.data);
    my_benchmarkP.personal_dbp->put(my_benchmarkP.personal_dbp, 0, &key, &data, 0);
  }

  fclose(ifp);
  return (0);
}

int
load_stocks_database(BENCHMARK_DBS my_benchmarkP, char *stocks_file)
{
  DBT key, data;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  STOCK my_stocks;

  printf("================= LOADING STOCKS DATABASE ==============\n");

  ifp = fopen(stocks_file, "r");
  if (ifp == NULL) {
    fprintf(stderr, "Error opening file '%s'\n", stocks_file);
    return (-1);
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
    printf("Inserting: %s\n", key.data);
    printf("\t(%s, %s)\n", my_stocks.stock_symbol, my_stocks.full_name);
    my_benchmarkP.stocks_dbp->put(my_benchmarkP.stocks_dbp, 0, &key, &data, 0);
  }

  fclose(ifp);
  return (0);
}

int
load_currencies_database(BENCHMARK_DBS my_benchmarkP, char *currencies_file)
{
  DBT key, data;
  char buf[MAXLINE];
  char ignore_buf[500];
  FILE *ifp;
  CURRENCY my_currencies;
  int rc = 0;
 
  printf("================= LOADING CURRENCIES DATABASE ==============\n");

  ifp = fopen(currencies_file, "r");
  if (ifp == NULL) {
    fprintf(stderr, "Error opening file '%s'\n", currencies_file);
    return (-1);
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
    printf("Inserting: %s\n", key.data);
    rc = my_benchmarkP.currencies_dbp->put(my_benchmarkP.currencies_dbp, 0, &key, &data, 0);
    if (rc != 0) {
      printf("Could not save register\n");
      goto failXit;
    }
  }

failXit:
  fclose(ifp);
  return (0);
}

