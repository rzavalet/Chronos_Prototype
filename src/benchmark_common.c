/*
 * =====================================================================================
 *
 *       Filename:  benchmark_common.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/14/16 19:31:17
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricardo Zavaleta (rj.zavaleta@gmail.com)
 *   Organization:  
 *
 * =====================================================================================
 */


#include "benchmark_common.h"

static int 
show_stock_item(void *);

/*=============== STATIC FUNCTIONS =======================*/
static int
show_stock_item(void *vBuf)
{
  char *symbol;
  char *name;

  size_t buf_pos = 0;
  char *buf = (char *)vBuf;

  /* TODO: Pack the string instead */
  symbol = buf;
  buf_pos += ID_SZ;

  name = buf + buf_pos;

  /* Display all this information */
  printf("Symbol: %s\n", symbol);
  printf("\tName: %s\n", name);

  /* Return the vendor's name */
  return 0;
}

static int
open_database(DB **dbpp,       
              const char *file_name,     
              const char *program_name,  
              FILE *error_file_pointer,
              int is_secondary)
{
  DB *dbp;
  u_int32_t open_flags;
  int ret;

  /* Initialize the DB handle */
  ret = db_create(&dbp, NULL, 0);
  if (ret != 0) {
    fprintf(error_file_pointer, "%s: %s\n", program_name,
    db_strerror(ret));
    return (ret);
  }
  /* Point to the memory malloc'd by db_create() */
  *dbpp = dbp;

  /* Set up error handling for this database */
  dbp->set_errfile(dbp, error_file_pointer);
  dbp->set_errpfx(dbp, program_name);

#if 0
  /*
   * If this is a secondary database, then we want to allow
   * sorted duplicates.
   */
  if (is_secondary) {
    ret = dbp->set_flags(dbp, DB_DUPSORT);
    if (ret != 0) {
        dbp->err(dbp, ret, "Attempt to set DUPSORT flags failed.",
          file_name);
        return (ret);
    }
  }
#endif

  /* Set the open flags */
  open_flags = DB_CREATE;    /* Allow database creation */

  /* Now open the database */
  ret = dbp->open(dbp,        /* Pointer to the database */
                  NULL,       /* Txn pointer */
                  file_name,  /* File name */
                  NULL,       /* Logical db name */
                  DB_BTREE,   /* Database type (using btree) */
                  open_flags, /* Open flags */
                  0);         /* File mode. Using defaults */
  if (ret != 0) {
    dbp->err(dbp, ret, "Database '%s' open failed.", file_name);
    return (ret);
  }

  return (0);
}

/*=============== PUBLIC FUNCTIONS =======================*/
int	
databases_setup(BENCHMARK_DBS *my_benchmarkP,
                int which_database,
                const char *program_name, 
                FILE *error_fileP)
{
  int ret;

  if (IS_STOCKS(which_database)) {
    ret = open_database(&(my_benchmarkP->stocks_dbp),
                        my_benchmarkP->stocks_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_QUOTES(which_database)) {
    ret = open_database(&(my_benchmarkP->quotes_dbp),
                        my_benchmarkP->quotes_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_QUOTES_HIST(which_database)) {
    ret = open_database(&(my_benchmarkP->quotes_hist_dbp),
                        my_benchmarkP->quotes_hist_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_PORTFOLIOS(which_database)) {
    ret = open_database(&(my_benchmarkP->portfolios_dbp),
                        my_benchmarkP->portfolios_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_ACCOUNTS(which_database)) {
    ret = open_database(&(my_benchmarkP->accounts_dbp),
                        my_benchmarkP->accounts_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_CURRENCIES(which_database)) {
    ret = open_database(&(my_benchmarkP->currencies_dbp),
                        my_benchmarkP->currencies_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_PERSONAL(which_database)) {
    ret = open_database(&(my_benchmarkP->personal_dbp),
                        my_benchmarkP->personal_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  printf("databases opened successfully\n");
  return (0);
}


/* Initializes the STOCK_DBS struct.*/
void
initialize_benchmarkdbs(BENCHMARK_DBS *my_benchmarkP)
{
  my_benchmarkP->db_home_dir = DEFAULT_HOMEDIR;

  my_benchmarkP->stocks_dbp = NULL;
  my_benchmarkP->quotes_dbp = NULL;
  my_benchmarkP->quotes_hist_dbp = NULL;
  my_benchmarkP->portfolios_dbp = NULL;
  my_benchmarkP->accounts_dbp = NULL;
  my_benchmarkP->currencies_dbp = NULL;
  my_benchmarkP->personal_dbp = NULL;

  my_benchmarkP->stocks_db_name = NULL;
  my_benchmarkP->quotes_db_name = NULL;
  my_benchmarkP->quotes_hist_db_name = NULL;
  my_benchmarkP->portfolios_db_name = NULL;
  my_benchmarkP->accounts_db_name = NULL;
  my_benchmarkP->currencies_db_name = NULL;
  my_benchmarkP->personal_db_name = NULL;
}

/* Identify all the files that will hold our databases. */
void
set_db_filenames(BENCHMARK_DBS *my_benchmarkP)
{
  size_t size;

  size = strlen(my_benchmarkP->db_home_dir) + strlen(STOCKSDB) + 1;
  my_benchmarkP->stocks_db_name = malloc(size);
  snprintf(my_benchmarkP->stocks_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, STOCKSDB);

  size = strlen(my_benchmarkP->db_home_dir) + strlen(QUOTESDB) + 1;
  my_benchmarkP->quotes_db_name = malloc(size);
  snprintf(my_benchmarkP->quotes_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, QUOTESDB);

  size = strlen(my_benchmarkP->db_home_dir) + strlen(QUOTES_HISTDB) + 1;
  my_benchmarkP->quotes_hist_db_name = malloc(size);
  snprintf(my_benchmarkP->quotes_hist_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, QUOTES_HISTDB);

  size = strlen(my_benchmarkP->db_home_dir) + strlen(PORTFOLIOSDB) + 1;
  my_benchmarkP->portfolios_db_name = malloc(size);
  snprintf(my_benchmarkP->portfolios_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, PORTFOLIOSDB);

  size = strlen(my_benchmarkP->db_home_dir) + strlen(ACCOUNTSDB) + 1;
  my_benchmarkP->accounts_db_name = malloc(size);
  snprintf(my_benchmarkP->accounts_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, ACCOUNTSDB);

  size = strlen(my_benchmarkP->db_home_dir) + strlen(CURRENCIESDB) + 1;
  my_benchmarkP->currencies_db_name = malloc(size);
  snprintf(my_benchmarkP->currencies_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, CURRENCIESDB);

  size = strlen(my_benchmarkP->db_home_dir) + strlen(PERSONALDB) + 1;
  my_benchmarkP->personal_db_name = malloc(size);
  snprintf(my_benchmarkP->personal_db_name, size, "%s%s",
          my_benchmarkP->db_home_dir, PERSONALDB);
}

int
databases_close(BENCHMARK_DBS *my_benchmarkP)
{
  int ret;

  if (my_benchmarkP->stocks_dbp != NULL) {
    ret = my_benchmarkP->stocks_dbp->close(my_benchmarkP->stocks_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (my_benchmarkP->quotes_dbp != NULL) {
    ret = my_benchmarkP->quotes_dbp->close(my_benchmarkP->quotes_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (my_benchmarkP->quotes_hist_dbp != NULL) {
    ret = my_benchmarkP->quotes_hist_dbp->close(my_benchmarkP->quotes_hist_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (my_benchmarkP->portfolios_dbp != NULL) {
    ret = my_benchmarkP->portfolios_dbp->close(my_benchmarkP->portfolios_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (my_benchmarkP->accounts_dbp != NULL) {
    ret = my_benchmarkP->accounts_dbp->close(my_benchmarkP->accounts_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (my_benchmarkP->currencies_dbp != NULL) {
    ret = my_benchmarkP->currencies_dbp->close(my_benchmarkP->currencies_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (my_benchmarkP->personal_dbp != NULL) {
    ret = my_benchmarkP->personal_dbp->close(my_benchmarkP->personal_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  printf("databases closed.\n");
  return (0);
}


int 
show_stocks_records(BENCHMARK_DBS *benchmarkP)
{
  DBC *stock_cursorp;
  DBT key, data;
  char *the_stock;
  int exit_value, ret;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  printf("================= SHOWING STOCKS DATABASE ==============\n");

  benchmarkP->stocks_dbp->cursor(benchmarkP->stocks_dbp, NULL,
                                    &stock_cursorp, 0);

  exit_value = 0;
  while ((ret =
    stock_cursorp->get(stock_cursorp, &key, &data, DB_NEXT)) == 0)
  {
    (void) show_stock_item(data.data);
  }

  stock_cursorp->close(stock_cursorp);
  return (exit_value);
}

