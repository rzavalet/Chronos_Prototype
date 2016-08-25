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
 *   Organization:  Cinvestav
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
open_database(DB_ENV *envP,
              DB **dbpp,       
              const char *file_name,     
              const char *program_name,  
              FILE *error_file_pointer,
              int is_secondary)
{
  DB *dbp;
  u_int32_t open_flags;
  int ret;

  /* Initialize the DB handle */
  ret = db_create(&dbp, envP, 0);
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
  open_flags = DB_CREATE |    /* Allow database creation */
               DB_AUTO_COMMIT; 

  /* Now open the database */
  ret = dbp->open(dbp,        /* Pointer to the database */
                  NULL,       /* Txn pointer */
                  file_name,  /* File name */
                  NULL,       /* Logical db name */
                  DB_BTREE,   /* Database type (using btree) */
                  open_flags, /* Open flags */
                  0);         /* File mode. Using defaults */
  if (ret != 0) {
    envP->err(envP, ret, "%d: Database '%s' open failed", __LINE__, file_name);
    return (ret);
  }

  return (0);
}

static int
close_database(DB_ENV *envP,
              DB *dbP,       
              const char *program_name)
{
  int rc = 0;

  if (envP == NULL || dbP == NULL) {
    fprintf(stderr, "%s: Invalid argument\n", __func__);
    goto failXit;
  } 

  rc = dbP->close(dbP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "Database close failed.");
    goto failXit;
  }

  goto cleanup;

failXit:
  rc = 1;

cleanup:
  return rc; 
}

int open_environment(BENCHMARK_DBS *benchmarkP)
{
  int rc = 0;
  u_int32_t env_flags;
  DB_ENV  *envP = NULL;

  if (benchmarkP == NULL || benchmarkP->envP == NULL) {
    fprintf(stderr, "%s: Invalid argument\n", __func__);
    goto failXit;
  }


  rc = db_env_create(&envP, 0);
  if (rc != 0) {
    fprintf(stderr, "Error creating environment handle: %s\n",
            db_strerror(rc));
    goto failXit;
  }
 
  env_flags = DB_CREATE    |
              DB_INIT_TXN  |
              DB_INIT_LOCK |
              DB_INIT_LOG  |
              DB_INIT_MPOOL;

  rc = envP->open(envP, benchmarkP->db_home_dir, env_flags, 0); 
  if (rc != 0) {
    fprintf(stderr, "Error opening environment: %s\n",
            db_strerror(rc));
    goto failXit;
  }

  goto cleanup;

failXit:
  if (envP != NULL) {
    rc = envP->close(envP, 0);
    if (rc != 0) {
      fprintf(stderr, "Error closing environment: %s\n",
              db_strerror(rc));
    }
  }
  rc = 1;

cleanup:
  benchmarkP->envP = envP;
  return rc;
}


/*=============== PUBLIC FUNCTIONS =======================*/
int	
databases_setup(BENCHMARK_DBS *benchmarkP,
                int which_database,
                const char *program_name, 
                FILE *error_fileP)
{
  int ret;

  /* TODO: Maybe better to receive the envP and pass the home dir? */
  ret = open_environment(benchmarkP);
  if (ret != 0) {
    fprintf(stderr, "%s: Could not open environment\n",
            __func__);
    goto failXit;
  }

  if (IS_STOCKS(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->stocks_dbp),
                        benchmarkP->stocks_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_QUOTES(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->quotes_dbp),
                        benchmarkP->quotes_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_QUOTES_HIST(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->quotes_hist_dbp),
                        benchmarkP->quotes_hist_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_PORTFOLIOS(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->portfolios_dbp),
                        benchmarkP->portfolios_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_ACCOUNTS(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->accounts_dbp),
                        benchmarkP->accounts_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_CURRENCIES(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->currencies_dbp),
                        benchmarkP->currencies_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_PERSONAL(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->personal_dbp),
                        benchmarkP->personal_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB);
    if (ret != 0) {
      return (ret);
    }
  }

  printf("databases opened successfully\n");
  return (0);

failXit:
  return 1;
}


int	
benchmark_end(BENCHMARK_DBS *benchmarkP,
              int which_database,
              char *program_name)
{
  int rc = 0;

  if (benchmarkP->envP == NULL) {
    fprintf(stderr, "%s: Invalid argument\n", __func__);
    goto failXit;
  } 

  if (IS_STOCKS(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->stocks_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  if (IS_QUOTES(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->quotes_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  if (IS_QUOTES_HIST(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->quotes_hist_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  if (IS_PORTFOLIOS(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->portfolios_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  if (IS_ACCOUNTS(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->accounts_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  if (IS_CURRENCIES(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->currencies_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  if (IS_PERSONAL(which_database)) {
    rc = close_database(benchmarkP->envP,
                        benchmarkP->personal_dbp,
                        program_name);
    if (rc != 0) {
      goto failXit;
    }
  }

  printf("databases opened successfully\n");
  return (0);

failXit:
  rc = 1;

cleanup:
  return rc; 
}

/* Initializes the STOCK_DBS struct.*/
void
initialize_benchmarkdbs(BENCHMARK_DBS *benchmarkP)
{
  benchmarkP->db_home_dir = NULL;

  benchmarkP->stocks_dbp = NULL;
  benchmarkP->quotes_dbp = NULL;
  benchmarkP->quotes_hist_dbp = NULL;
  benchmarkP->portfolios_dbp = NULL;
  benchmarkP->accounts_dbp = NULL;
  benchmarkP->currencies_dbp = NULL;
  benchmarkP->personal_dbp = NULL;

  benchmarkP->stocks_db_name = NULL;
  benchmarkP->quotes_db_name = NULL;
  benchmarkP->quotes_hist_db_name = NULL;
  benchmarkP->portfolios_db_name = NULL;
  benchmarkP->accounts_db_name = NULL;
  benchmarkP->currencies_db_name = NULL;
  benchmarkP->personal_db_name = NULL;
}

/* Identify all the files that will hold our databases. */
void
set_db_filenames(BENCHMARK_DBS *benchmarkP)
{
  size_t size;

  size = strlen(STOCKSDB) + 1;
  benchmarkP->stocks_db_name = malloc(size);
  snprintf(benchmarkP->stocks_db_name, size, "%s", STOCKSDB);

  size = strlen(QUOTESDB) + 1;
  benchmarkP->quotes_db_name = malloc(size);
  snprintf(benchmarkP->quotes_db_name, size, "%s", QUOTESDB);

  size = strlen(QUOTES_HISTDB) + 1;
  benchmarkP->quotes_hist_db_name = malloc(size);
  snprintf(benchmarkP->quotes_hist_db_name, size, "%s", QUOTES_HISTDB);

  size = strlen(PORTFOLIOSDB) + 1;
  benchmarkP->portfolios_db_name = malloc(size);
  snprintf(benchmarkP->portfolios_db_name, size, "%s", PORTFOLIOSDB);

  size = strlen(ACCOUNTSDB) + 1;
  benchmarkP->accounts_db_name = malloc(size);
  snprintf(benchmarkP->accounts_db_name, size, "%s", ACCOUNTSDB);

  size = strlen(CURRENCIESDB) + 1;
  benchmarkP->currencies_db_name = malloc(size);
  snprintf(benchmarkP->currencies_db_name, size, "%s", CURRENCIESDB);

  size = strlen(PERSONALDB) + 1;
  benchmarkP->personal_db_name = malloc(size);
  snprintf(benchmarkP->personal_db_name, size, "%s", PERSONALDB);
}

int
databases_close(BENCHMARK_DBS *benchmarkP)
{
  int ret;

  if (benchmarkP->stocks_dbp != NULL) {
    ret = benchmarkP->stocks_dbp->close(benchmarkP->stocks_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (benchmarkP->quotes_dbp != NULL) {
    ret = benchmarkP->quotes_dbp->close(benchmarkP->quotes_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (benchmarkP->quotes_hist_dbp != NULL) {
    ret = benchmarkP->quotes_hist_dbp->close(benchmarkP->quotes_hist_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (benchmarkP->portfolios_dbp != NULL) {
    ret = benchmarkP->portfolios_dbp->close(benchmarkP->portfolios_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (benchmarkP->accounts_dbp != NULL) {
    ret = benchmarkP->accounts_dbp->close(benchmarkP->accounts_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (benchmarkP->currencies_dbp != NULL) {
    ret = benchmarkP->currencies_dbp->close(benchmarkP->currencies_dbp, 0);
    if (ret != 0) {
      fprintf(stderr, "Inventory database close failed: %s\n", db_strerror(ret));
    }
  }

  if (benchmarkP->personal_dbp != NULL) {
    ret = benchmarkP->personal_dbp->close(benchmarkP->personal_dbp, 0);
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

