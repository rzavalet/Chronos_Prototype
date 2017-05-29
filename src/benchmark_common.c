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

char *symbolsArr[] = {"YHOO", "AAPL", "GOOG", "MSFT", "PIH", "FLWS", "SRCE", "VNET", "TWOU", "JOBS"};

static int
symbol_exists(char *symbol, DB_TXN *txnP, BENCHMARK_DBS *benchmarkP);

static int
account_exists(char *account_id, DB_TXN *txnP, BENCHMARK_DBS *benchmarkP);

static int 
show_stock_item(void *);

static int
show_currencies_item(void *vBuf);

static int
get_account_id(DB *sdbp,          /* secondary db handle */
              const DBT *pkey,   /* primary db record's key */
              const DBT *pdata,  /* primary db record's data */
              DBT *skey);         /* secondary db record's key */

static int 
create_portfolio(char *account_id, char *symbol, float price, int amount, DB_TXN *txnP, BENCHMARK_DBS *benchmarkP);

int
get_portfolio(char *account_id, char *symbol, DB_TXN *txnP, DBC **cursorPP, DBT *key_ret, DBT *data_ret, BENCHMARK_DBS *benchmarkP);

int
get_stock(char *symbol, DB_TXN *txnP, DBC **cursorPP, DBT *key_ret, DBT *data_ret, int flags, BENCHMARK_DBS *benchmarkP);

/*=============== STATIC FUNCTIONS =======================*/
static int
get_account_id(DB *sdbp,          /* secondary db handle */
              const DBT *pkey,   /* primary db record's key */
              const DBT *pdata,  /* primary db record's data */
              DBT *skey)         /* secondary db record's key */
{
    PORTFOLIOS *portfoliosP;

    /* First, extract the structure contained in the primary's data */
    portfoliosP = pdata->data;

    /* Now set the secondary key's data to be the representative's name */
    memset(skey, 0, sizeof(DBT));
    skey->data = portfoliosP->account_id;
    skey->size = strlen(portfoliosP->account_id) + 1;

    /* Return 0 to indicate that the record can be created/updated. */
    return (0);
} 

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
  benchmark_debug(5,"Symbol: %s", symbol);
  benchmark_debug(5,"\tName: %s", name);

  /* Return the vendor's name */
  return 0;
}

static int
open_database(DB_ENV *envP,
              DB **dbpp,       
              const char *file_name,     
              const char *program_name,  
              FILE *error_file_pointer,
              int is_secondary,
              int create)
{
  DB *dbp;
  u_int32_t open_flags;
  int ret;

  /* Initialize the DB handle */
  ret = db_create(&dbp, envP, 0);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Could not create db handle.", __FILE__, __LINE__, getpid());
    return (ret);
  }
  /* Point to the memory malloc'd by db_create() */
  *dbpp = dbp;

  /* Set up error handling for this database */
  dbp->set_errfile(dbp, error_file_pointer);
  dbp->set_errpfx(dbp, program_name);

  /*
   * If this is a secondary database, then we want to allow
   * sorted duplicates.
   */
  if (is_secondary) {
    ret = dbp->set_flags(dbp, DB_DUPSORT);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Attempt to set DUPSORT flags failed.", __FILE__, __LINE__, getpid());
      return (ret);
    }
  }

  /* Set the open flags */
  open_flags = DB_AUTO_COMMIT; 

  if (create)
    open_flags |= DB_CREATE; /*  Allow database creation */

  /* Now open the database */
  ret = dbp->open(dbp,        /* Pointer to the database */
                  NULL,       /* Txn pointer */
                  file_name,  /* File name */
                  NULL,       /* Logical db name */
                  DB_BTREE,   /* Database type (using btree) */
                  open_flags, /* Open flags */
                  0);         /* File mode. Using defaults */
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d]: Database '%s' open failed", __FILE__, __LINE__, getpid(), file_name);
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
    benchmark_error("Invalid argument");
    goto failXit;
  } 

  rc = dbP->close(dbP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Database close failed.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  goto cleanup;

failXit:
  rc = 1;

cleanup:
  return rc; 
}

int close_environment(BENCHMARK_DBS *benchmarkP)
{
  int rc = 0;
  DB_ENV  *envP = NULL;

  if (benchmarkP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }


  rc = envP->close(envP, 0);
  if (rc != 0) {
    benchmark_error("Error closing environment: %s", db_strerror(rc));
  }

  benchmarkP->envP = NULL;
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

  if (benchmarkP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  rc = db_env_create(&envP, 0);
  if (rc != 0) {
    benchmark_error("Error creating environment handle: %s", db_strerror(rc));
    goto failXit;
  }
 
  env_flags = DB_INIT_TXN  |
              DB_INIT_LOCK |
              DB_INIT_LOG  |
              DB_INIT_MPOOL;

  if (benchmarkP->createDBs == 1) 
    env_flags |= DB_CREATE;

#ifdef CHRONOS_INMEMORY
 env_flags |= DB_SYSTEM_MEM;

  /*
   * Indicate that we want db to perform lock detection internally.
   * Also indicate that the transaction with the fewest number of
   * write locks will receive the deadlock notification in 
   * the event of a deadlock.
   */  
  rc = envP->set_lk_detect(envP, DB_LOCK_MINWRITE);
  if (rc != 0) {
      benchmark_error("Error setting lock detect: %s", db_strerror(rc));
      goto failXit;
  } 

  rc = envP->set_shm_key(envP, CHRONOS_SHMKEY); 
  if (rc != 0) {
      benchmark_error("Error setting shm key: %s", db_strerror(rc));
      goto failXit;
  } 
#ifdef CHRONOS_DEBUG
  printf("shm key set to: %d\n", CHRONOS_SHMKEY); 
#endif
#endif

  rc = envP->open(envP, benchmarkP->db_home_dir, env_flags, 0); 
  if (rc != 0) {
    benchmark_error("Error opening environment: %s", db_strerror(rc));
    goto failXit;
  }

  goto cleanup;

failXit:
  if (envP != NULL) {
    rc = envP->close(envP, 0);
    if (rc != 0) {
      benchmark_error("Error closing environment: %s", db_strerror(rc));
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
  DB_ENV  *envP = NULL;

  if (benchmarkP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  /* TODO: Maybe better to receive the envP and pass the home dir? */
  ret = open_environment(benchmarkP);
  if (ret != 0) {
    benchmark_error("Could not open environment");
    goto failXit;
  }

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  if (IS_STOCKS(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->stocks_dbp),
                        benchmarkP->stocks_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_QUOTES(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->quotes_dbp),
                        benchmarkP->quotes_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_QUOTES_HIST(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->quotes_hist_dbp),
                        benchmarkP->quotes_hist_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_PORTFOLIOS(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->portfolios_dbp),
                        benchmarkP->portfolios_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }

    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->portfolios_sdbp),
                        benchmarkP->portfolios_sdb_name,
                        program_name, error_fileP,
                        SECONDARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }

    /* Now associate the secondary to the primary */
    ret = benchmarkP->portfolios_dbp->associate(benchmarkP->portfolios_dbp,            /* Primary database */
                   NULL,           /* TXN id */
                   benchmarkP->portfolios_sdbp,           /* Secondary database */
                   get_account_id,
                   0);

    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Failed to associate secondary database.", __FILE__, __LINE__, getpid());
      return (ret);
    }

  }

  if (IS_ACCOUNTS(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->accounts_dbp),
                        benchmarkP->accounts_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_CURRENCIES(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->currencies_dbp),
                        benchmarkP->currencies_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }
  }

  if (IS_PERSONAL(which_database)) {
    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->personal_dbp),
                        benchmarkP->personal_db_name,
                        program_name, error_fileP,
                        PRIMARY_DB,
                        benchmarkP->createDBs);
    if (ret != 0) {
      return (ret);
    }
  }

#ifdef CHRONOS_DEBUG
  printf("databases opened successfully\n");
#endif
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

#ifdef CHRONOS_DEBUG
  benchmark_debug(2,"databases opened successfully\n");
#endif
  return (0);

failXit:
  return 1; 
}

/* Initializes the STOCK_DBS struct.*/
void
initialize_benchmarkdbs(BENCHMARK_DBS *benchmarkP)
{
  memset(benchmarkP, 0, sizeof(BENCHMARK_DBS));
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

  size = strlen(PORTFOLIOSSECDB) + 1;
  benchmarkP->portfolios_sdb_name = malloc(size);
  snprintf(benchmarkP->portfolios_sdb_name, size, "%s", PORTFOLIOSSECDB);

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

  DB_ENV  *envP = NULL;

  if (benchmarkP == NULL) {
    fprintf(stderr, "%s:%d Invalid argument\n", __func__, __LINE__);
    goto failXit;
  }

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  if (benchmarkP->stocks_dbp != NULL) {
    ret = benchmarkP->stocks_dbp->close(benchmarkP->stocks_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Stocks database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  if (benchmarkP->quotes_dbp != NULL) {
    ret = benchmarkP->quotes_dbp->close(benchmarkP->quotes_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Quotes database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  if (benchmarkP->quotes_hist_dbp != NULL) {
    ret = benchmarkP->quotes_hist_dbp->close(benchmarkP->quotes_hist_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Quotes History database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  if (benchmarkP->portfolios_dbp != NULL) {
    ret = benchmarkP->portfolios_dbp->close(benchmarkP->portfolios_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Portfolios database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  if (benchmarkP->accounts_dbp != NULL) {
    ret = benchmarkP->accounts_dbp->close(benchmarkP->accounts_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Accounts database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  if (benchmarkP->currencies_dbp != NULL) {
    ret = benchmarkP->currencies_dbp->close(benchmarkP->currencies_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Currencies database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  if (benchmarkP->personal_dbp != NULL) {
    ret = benchmarkP->personal_dbp->close(benchmarkP->personal_dbp, 0);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Personal database close failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
#ifdef CHRONOS_DEBUG
  benchmark_debug(2,"databases closed.\n");
#endif
  return (0);

failXit:
  return 1;
}


int 
show_stocks_records(char *symbolId, BENCHMARK_DBS *benchmarkP)
{
  DBC *cursorP = NULL;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT key, data;
  int ret;
  int rc = BENCHMARK_SUCCESS;

  if (benchmarkP==NULL|| symbolId == NULL)
  {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  if (symbolId != NULL && symbolId[0] != '\0') {
    key.data = symbolId;
    key.size = (u_int32_t)strlen(symbolId) + 1;
  }

  ret = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Transaction begin failed.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  ret = benchmarkP->stocks_dbp->cursor(benchmarkP->stocks_dbp, txnP,
                                    &cursorP, DB_READ_COMMITTED);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to create cursor for Stocks.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

#if 0
  while ((ret = cursorP->get(cursorP, &key, &data, DB_NEXT)) == 0)
  {
    /*Uhhh, this is ugly.... */
    if (symbolId != NULL) {
      if (strcmp(symbolId, (char *)key.data) == 0) {
        (void) show_stock_item(data.data);
      }
    }
    else {
      (void) show_stock_item(data.data);
    }
  }
#endif
  
  /* Position the cursor */
  ret = cursorP->get(cursorP, &key, &data, DB_SET | DB_READ_COMMITTED | DB_RMW );
  if (ret != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to find record in Quotes.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  if (symbolId != NULL) {
    if (strcmp(symbolId, (char *)key.data) == 0) {
      (void) show_stock_item(data.data);
    }
  }
  else {
    (void) show_stock_item(data.data);
  }

  ret = cursorP->close(cursorP);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
  }
  cursorP = NULL;

  benchmark_info("PID: %d, Committing transaction: %p", getpid(), txnP);
  ret = txnP->commit(txnP, 0);
  if (ret != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction commit failed. txnP: %p", __FILE__, __LINE__, getpid(), txnP);
    goto failXit; 
  }
  txnP = NULL;

  goto cleanup;

failXit:
  rc = BENCHMARK_FAIL;
  if (txnP != NULL) {
    if (cursorP != NULL) {
      ret = cursorP->close(cursorP);
      if (ret != 0) {
        envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
      }
      cursorP = NULL;
    }

    benchmark_info("PID: %d About to abort transaction. txnP: %p", getpid(), txnP);
    ret = txnP->abort(txnP);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
    }
  }


cleanup:
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  return rc;
}

int 
show_all_portfolios(BENCHMARK_DBS *benchmarkP)
{
  DBC *cursorP = NULL;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  char *symbolIdP = NULL;
  DBT key, data;
  int ret;
  int rc = BENCHMARK_SUCCESS;
  int curRc = 0;
  int numClients = 0;

  if (benchmarkP == NULL || benchmarkP->personal_dbp == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  ret = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Transaction begin failed.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  /* First read the portfolios account */
  ret = benchmarkP->portfolios_dbp->cursor(benchmarkP->portfolios_dbp, txnP,
                                    &cursorP, DB_READ_COMMITTED);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to create cursor for Personal.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  while ((curRc=cursorP->get(cursorP, &key, &data, DB_READ_COMMITTED | DB_NEXT)) == 0)
  {
#ifdef CHRONOS_DEBUG
     benchmark_debug(4,"================= SHOWING PORTFOLIO ==============\n");
#endif
    (void) show_portfolio_item(data.data, &symbolIdP);
#ifdef CHRONOS_DEBUG
     benchmark_debug(4,"==================================================\n");
#endif
  }

  ret = cursorP->close(cursorP);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
  }
  cursorP = NULL;

  benchmark_info("PID: %d, Committing transaction: %p", getpid(), txnP);
  ret = txnP->commit(txnP, 0);
  if (ret != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction commit failed. txnP: %p", __FILE__, __LINE__, getpid(), txnP);
    goto failXit; 
  }
  txnP = NULL;

#ifdef CHRONOS_DEBUG
  benchmark_debug(3,"Displayed info about %d clients\n", numClients);
#endif

  goto cleanup;

failXit:
  rc = BENCHMARK_FAIL;
  if (txnP != NULL) {
    if (cursorP != NULL) {
      ret = cursorP->close(cursorP);
      if (ret != 0) {
        envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
      }
      cursorP = NULL;
    }

    benchmark_info("PID: %d About to abort transaction. txnP: %p", getpid(), txnP);
    ret = txnP->abort(txnP);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
    }
  }

cleanup:
  return (rc);
}

int 
show_portfolios(BENCHMARK_DBS *benchmarkP)
{
  DBC *personal_cursorP = NULL;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT key, data;
  int ret;
  int rc = BENCHMARK_SUCCESS;
  int curRc = 0;
  int numClients = 0;

  if (benchmarkP == NULL || benchmarkP->personal_dbp == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  ret = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Transaction begin failed.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  /* First read the personall account */
  ret = benchmarkP->personal_dbp->cursor(benchmarkP->personal_dbp, txnP,
                                    &personal_cursorP, DB_READ_COMMITTED);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to create cursor for Personal.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  while ((curRc=personal_cursorP->get(personal_cursorP, &key, &data, DB_READ_COMMITTED | DB_NEXT)) == 0)
  {
     benchmark_debug(4,"================= SHOWING PORTFOLIO ==============");

    (void) show_personal_item(data.data);   
    ret = show_one_portfolio(key.data, txnP, benchmarkP);
    if (ret != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to retrieve portfolio");
      //goto failXit;
    }
    numClients ++;

    benchmark_debug(4,"==================================================\n");
  }

  if (curRc != DB_NOTFOUND) {
    envP->err(envP, ret, "[%s:%d] [%d] Error retrieving portfolios.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  ret = personal_cursorP->close(personal_cursorP);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
  }
  personal_cursorP = NULL;

  benchmark_info("PID: %d, Committing transaction: %p", getpid(), txnP);
  ret = txnP->commit(txnP, 0);
  if (ret != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction commit failed. txnP: %p", __FILE__, __LINE__, getpid(), txnP);
    goto failXit; 
  }
  txnP = NULL;

#ifdef CHRONOS_DEBUG
  benchmark_debug(3,"Displayed info about %d clients\n", numClients);
#endif

  goto cleanup;

failXit:
  rc = BENCHMARK_FAIL;
  if (txnP != NULL) {
    if (personal_cursorP != NULL) {
      ret = personal_cursorP->close(personal_cursorP);
      if (ret != 0) {
        envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
      }
      personal_cursorP = NULL;
    }

    benchmark_info("PID: %d About to abort transaction. txnP: %p", getpid(), txnP);
    ret = txnP->abort(txnP);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
    }
  }

cleanup:
  return (rc);
}

int
show_one_portfolio(char *account_id, DB_TXN  *txn_inP, BENCHMARK_DBS *benchmarkP)
{
  DBC *portfolio_cursorP = NULL;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT key;
  DBT pkey, pdata;
  char *symbolIdP = NULL;
  int rc = BENCHMARK_SUCCESS;
  int ret;
  int numPortfolios = 0;

  if (benchmarkP == NULL || benchmarkP->portfolios_sdbp == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  if (txn_inP != NULL) {
    txnP = txn_inP;
  }
  else {
    ret = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
    if (ret != 0) {
      envP->err(envP, ret, "[%s:%d] [%d] Transaction begin failed.", __FILE__, __LINE__, getpid());
      goto failXit;
    }
  }

  memset(&key, 0, sizeof(DBT));
  memset(&pkey, 0, sizeof(DBT));
  memset(&pdata, 0, sizeof(DBT));

  key.data = account_id;
  key.size = ID_SZ;
  
  ret = benchmarkP->portfolios_sdbp->cursor(benchmarkP->portfolios_sdbp, txnP, 
                                   &portfolio_cursorP, DB_READ_COMMITTED);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to create cursor for Portfolios.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  benchmark_info("PID: %d, %p : searching for account id: %s", getpid(), txnP, account_id);

#if 1
  while ((ret = portfolio_cursorP->pget(portfolio_cursorP, &key, &pkey, &pdata, DB_NEXT)) == 0)
  {
    /* TODO: Is it necessary this comparison? */
    if (strcmp(account_id, (char *)key.data) == 0) {
      (void) show_portfolio_item(pdata.data, &symbolIdP);

#if 0
      if (symbolIdP != NULL && symbolIdP[0] != '\0') {
        (void) show_stocks_records(symbolIdP, benchmarkP);
      }
#endif
      numPortfolios ++;
    }
  }

#else 

  /* Position the cursor */
  rc = portfolio_cursorP->pget(portfolio_cursorP, &key, &pkey, &pdata, DB_SET | DB_READ_COMMITTED);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to find record (%s) in Portfolio.", __FILE__, __LINE__, getpid(), account_id);
    goto failXit;
  }

  (void) show_portfolio_item(pdata.data, &symbolIdP);

#endif
  ret = portfolio_cursorP->close(portfolio_cursorP);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
  }

  portfolio_cursorP = NULL;

  /* This means this function created its own txn */
  if (txn_inP == NULL) {
    benchmark_info("PID: %d, Committing transaction: %p", getpid(), txnP);
    ret = txnP->commit(txnP, 0);
    if (ret != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Transaction commit failed. txnP: %p", __FILE__, __LINE__, getpid(), txnP);
      goto failXit; 
    }
    txnP = NULL;
  }

#ifdef CHRONOS_DEBUG
  benchmark_debug(3,"Displayed info about %d portfolios\n", numPortfolios);
#endif
  goto cleanup;

failXit:
  rc = BENCHMARK_FAIL;
  if (txnP != NULL) {
    if (portfolio_cursorP != NULL) {
      ret = portfolio_cursorP->close(portfolio_cursorP);
      if (ret != 0) {
        envP->err(envP, ret, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
      }
      portfolio_cursorP = NULL;
    }

    /* This means this function created its own txn */
    if (txn_inP == NULL) {
      benchmark_info("PID: %d About to abort transaction. txnP: %p", getpid(), txnP);
      ret = txnP->abort(txnP);
      if (ret != 0) {
        envP->err(envP, ret, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
      }
    }
  }

cleanup:
  return (rc);
}

int
show_personal_item(void *vBuf)
{
  char      *account_id;
  char      *last_name;
  char      *first_name;
  char      *address;
  char      *address_2;
  char      *city;
  char      *state;
  char      *country;
  char      *phone;
  char      *email;

  size_t buf_pos = 0;
  char *buf = (char *)vBuf;

  account_id = buf;
  buf_pos += ID_SZ;
  
  last_name = buf + buf_pos;
  buf_pos += NAME_SZ;

  first_name = buf + buf_pos;
  buf_pos += NAME_SZ;

  address = buf + buf_pos;
  buf_pos += NAME_SZ;

  address_2 = buf + buf_pos;
  buf_pos += NAME_SZ;

  city = buf + buf_pos;
  buf_pos += NAME_SZ;

  state = buf + buf_pos;
  buf_pos += NAME_SZ;

  country = buf + buf_pos;
  buf_pos += NAME_SZ;

  phone = buf + buf_pos;
  buf_pos += NAME_SZ;

  email = buf + buf_pos;
 
  /* Display all this information */
  benchmark_debug(5,"AccountId: %s", account_id);
  benchmark_debug(5,"\tLast Name: %s", last_name);
  benchmark_debug(5,"\tFirst Name: %s", first_name);
  benchmark_debug(5,"\tAddress: %s", address);
  benchmark_debug(5,"\tAdress 2: %s", address_2);
  benchmark_debug(5,"\tCity: %s", city);
  benchmark_debug(5,"\tState: %s", state);
  benchmark_debug(5,"\tCountry: %s", country);
  benchmark_debug(5,"\tPhone: %s", phone);
  benchmark_debug(5,"\tEmail: %s", email);

  return 0;
}


int 
show_currencies_records(BENCHMARK_DBS *my_benchmarkP)
{
  DBC *currencies_cursorp = NULL;
  DBT key, data;
  int exit_value, ret;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  benchmark_debug(4,"================= SHOWING CURRENCIES DATABASE ==============\n");

  my_benchmarkP->currencies_dbp->cursor(my_benchmarkP->currencies_dbp, NULL,
                                    &currencies_cursorp, 0);

  exit_value = 0;
  while ((ret =
    currencies_cursorp->get(currencies_cursorp, &key, &data, DB_NEXT)) == 0)
  {
    (void) show_currencies_item(data.data);
  }

  currencies_cursorp->close(currencies_cursorp);
  return (exit_value);
}

static int
show_currencies_item(void *vBuf)
{
  char      *currency_symbol;
  int       exchange_rate_usd;
  char      *country;
  char      *currency_name;

  size_t buf_pos = 0;
  char *buf = (char *)vBuf;

  currency_symbol = buf;
  buf_pos += ID_SZ;
  
  country = buf + buf_pos;
  buf_pos += LONG_NAME_SZ;

  currency_name = buf + buf_pos;
  buf_pos += NAME_SZ;

  exchange_rate_usd = *((int *)(buf + buf_pos));

  /* Display all this information */
  benchmark_debug(5,"Currency Symbol: %s", currency_symbol);
  benchmark_debug(5,"\tExchange Rate: %d", exchange_rate_usd);
  benchmark_debug(5,"\tCountry: %s", country);
  benchmark_debug(5,"\tName: %s", currency_name);

  return 0;
}

int
show_portfolio_item(void *vBuf, char **symbolIdPP)
{
  PORTFOLIOS *portfolioP;

  portfolioP = (PORTFOLIOS *)vBuf;
  /* Display all this information */
  benchmark_debug(5,"Portfolio ID: %s", portfolioP->portfolio_id);
  benchmark_debug(5,"\tAccount ID: %s", portfolioP->account_id);
  benchmark_debug(5,"\tSymbol ID: %s", portfolioP->symbol);
  benchmark_debug(5,"\t# Stocks Hold: %d", portfolioP->hold_stocks);
  benchmark_debug(5,"\tSell?: %d", portfolioP->to_sell);
  benchmark_debug(5,"\t# Stocks to sell: %d", portfolioP->number_sell);
  benchmark_debug(5,"\tPrice to sell: %d", portfolioP->price_sell);
  benchmark_debug(5,"\tBuy?: %d", portfolioP->to_buy);
  benchmark_debug(5,"\t# Stocks to buy: %d", portfolioP->number_buy);
  benchmark_debug(5,"\tPrice to buy: %d", portfolioP->price_buy);

  if (symbolIdPP) {
    *symbolIdPP = portfolioP->symbol; 
  }

  return 0;
}


int 
show_quote(char *symbolP, BENCHMARK_DBS *benchmarkP)
{
  int rc = BENCHMARK_SUCCESS;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT      key, data;
  DBC     *cursorp = NULL; /* To iterate over the porfolios */
  QUOTE   *quoteP = NULL;

  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  rc = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction begin failed.", __FILE__, __LINE__, getpid());
    goto failXit; 
  }

  benchmark_info("PID: %d, Starting transaction: %p", getpid(), txnP);
  rc = get_stock(symbolP, txnP, &cursorp, &key, &data, 0, benchmarkP);
  if (rc != BENCHMARK_SUCCESS) {
    benchmark_error("Could not find record.");
    goto failXit; 
  }
  
  quoteP = data.data;
  benchmark_info("*** Value of %s is %f", quoteP->symbol, quoteP->current_price);

  /* Close the record */
  if (cursorp != NULL) {
    rc = cursorp->close(cursorp);
    if (rc != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Failed to close cursor for quote", __FILE__, __LINE__, getpid());
      goto failXit; 
    }
    cursorp = NULL;
  }

  benchmark_info("PID: %d, Committing transaction: %p", getpid(), txnP);
  rc = txnP->commit(txnP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction commit failed. txnP: %p", __FILE__, __LINE__, getpid(), txnP);
    goto failXit; 
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  goto cleanup;

failXit:
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  if (txnP != NULL) {
    if (cursorp != NULL) {
      rc = cursorp->close(cursorp);
      if (rc != 0) {
        envP->err(envP, rc, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
      }
      cursorp = NULL;
    }

    benchmark_info("PID: %d About to abort transaction. txnP: %p", getpid(), txnP);
    rc = txnP->abort(txnP);
    if (rc != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
    }
  }

  rc = BENCHMARK_FAIL;

cleanup:
  return rc;
}

int 
update_stock(char *symbolP, float newValue, BENCHMARK_DBS *benchmarkP)
{
  int rc = BENCHMARK_SUCCESS;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT      key, data;
  DBC     *cursorp = NULL; /* To iterate over the porfolios */
  QUOTE   *quoteP = NULL;

  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  rc = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction begin failed.", __FILE__, __LINE__, getpid());
    goto failXit; 
  }

  benchmark_info("PID: %d, Starting transaction: %p", getpid(), txnP);
  rc = get_stock(symbolP, txnP, &cursorp, &key, &data, DB_RMW, benchmarkP);
  if (rc != BENCHMARK_SUCCESS) {
    benchmark_error("Could not find record.");
    goto failXit; 
  }
  
  /* Update whatever we need to update */
  quoteP = data.data;
  if (newValue >= 0) {
    quoteP->current_price = newValue;
  }
  else {
    quoteP->current_price += 0.5;
  }

  benchmark_info("PID: %d, txnP: %p Updating %s to %f", getpid(), txnP, quoteP->symbol, quoteP->current_price);

  /* Save the record */
  rc = cursorp->put(cursorp, &key, &data, DB_CURRENT);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] failed to update quote", __FILE__, __LINE__, getpid());
    goto failXit; 
  }

  /* Close the record */
  if (cursorp != NULL) {
    rc = cursorp->close(cursorp);
    if (rc != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Failed to close cursor for quote", __FILE__, __LINE__, getpid());
      goto failXit; 
    }
    cursorp = NULL;
  }

  benchmark_info("PID: %d, Committing transaction: %p", getpid(), txnP);
  rc = txnP->commit(txnP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Transaction commit failed. txnP: %p", __FILE__, __LINE__, getpid(), txnP);
    goto failXit; 
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  goto cleanup;

failXit:
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  if (txnP != NULL) {
    if (cursorp != NULL) {
      rc = cursorp->close(cursorp);
      if (rc != 0) {
        envP->err(envP, rc, "[%s:%d] [%d] Failed to close cursor.", __FILE__, __LINE__, getpid());
      }
      cursorp = NULL;
    }

    benchmark_info("PID: %d About to abort transaction. txnP: %p", getpid(), txnP);
    rc = txnP->abort(txnP);
    if (rc != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
    }
  }

  rc = BENCHMARK_FAIL;

cleanup:
  return rc;
}


int 
sell_stocks(int account, char *symbol, float price, int amount, BENCHMARK_DBS *benchmarkP)
{
  int rc = 0;
  DB_TXN  *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT      skey, data;
  DBC     *cursorp = NULL; /* To iterate over the porfolios */
  char     account_id[ID_SZ];
  int      exists = 0;
  PORTFOLIOS portfolio;

  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  memset(&skey, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  sprintf(account_id, "%d", account);

  rc = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (rc != 0) {
    envP->err(envP, rc, "Transaction begin failed.");
    goto failXit; 
  }

  /* 1) search the account */
  exists = account_exists(account_id, txnP, benchmarkP);
  if (!exists) {
    benchmark_error("This user account does not exist.");
    goto failXit; 
  }

  /* 2) search the symbol */
  exists = symbol_exists(symbol, txnP, benchmarkP);
  if (!exists) {
    benchmark_error("This symbol does not exist.");
    goto failXit; 
  }

  benchmark_info("Looking up portfolio for account: %s and symbol: %s", account_id, symbol);

  rc = get_portfolio(account_id, symbol, txnP, &cursorp, &skey, &data, benchmarkP);
  if (rc != 0) {
    benchmark_error("Failed to obtain portfolio.");
    goto failXit; 
  }

  /* Update whatever we need to update */
  memcpy(&portfolio, &data.data, sizeof(PORTFOLIOS));
  benchmark_info("Found portfolio for account: %s and symbol: %s -> %s", account_id, symbol, portfolio.portfolio_id);
  if (portfolio.hold_stocks < amount) {
    benchmark_error("Not enough stocks for this symbol. Have: %d, wanted: %d", portfolio.hold_stocks, amount);
    goto failXit; 
  }
  portfolio.to_sell = 1;
  portfolio.number_sell = amount;
  portfolio.price_sell = price;

  /* Save the record */
  rc = cursorp->put(cursorp, &skey, &data, DB_CURRENT);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Could not update record.", __FILE__, __LINE__, getpid());
    goto failXit; 
  }

  /* Close the record */
  if (cursorp != NULL) {
    rc = cursorp->close(cursorp);
    if (rc != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Could not close cursor.", __FILE__, __LINE__, getpid());
      goto failXit; 
    }

    cursorp = NULL;
  }

  rc = txnP->commit(txnP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "%s:%d Transaction commit failed.", __func__, __LINE__);
    goto failXit; 
  }

  rc = BENCHMARK_SUCCESS;
  goto cleanup;

failXit:
  if (txnP != NULL) {
    if (cursorp != NULL) {
      rc = cursorp->close(cursorp);
      if (rc != 0) {
        envP->err(envP, rc, "[%s:%d] [%d] Could not close cursor.", __FILE__, __LINE__, getpid());
      }
      cursorp = NULL;
    }

    rc = txnP->abort(txnP);
    if (rc != 0) {
      envP->err(envP, rc, "[%s:%d] [%d] Transaction abort failed.", __FILE__, __LINE__, getpid());
      goto failXit; 
    }
  }

  rc = BENCHMARK_FAIL;

cleanup:
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  return rc;
}

int 
place_order(int account, char *symbol, float price, int amount, BENCHMARK_DBS *benchmarkP)
{
  int rc = 0;
  DB_TXN *txnP = NULL;
  DB_ENV  *envP = NULL;
  DBT key, data;
  char    account_id[ID_SZ];
  int exists = 0;

  envP = benchmarkP->envP;
  if (envP == NULL) {
    fprintf(stderr, "%s: Invalid arguments\n", __func__);
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  sprintf(account_id, "%d", account);

  rc = envP->txn_begin(envP, NULL, &txnP, DB_READ_COMMITTED | DB_TXN_WAIT);
  if (rc != 0) {
    envP->err(envP, rc, "Transaction begin failed.");
    goto failXit; 
  }

  /* 1) search the account */
  exists = account_exists(account_id, txnP, benchmarkP);
  if (!exists) {
    fprintf(stderr, "%s: This user account does not exist.\n", __func__);
    goto failXit; 
  }

  /* 2) search the symbol */
  exists = symbol_exists(symbol, txnP, benchmarkP);
  if (!exists) {
    fprintf(stderr, "%s: This symbol does not exist.\n", __func__);
    goto failXit; 
  }

  /* 3) exists portfolio */
  /* 3.1) if so, update */
  /* 3.2) otherwise, create a new portfolio */
  rc = create_portfolio(account_id, symbol, price, amount, txnP, benchmarkP);

  rc = txnP->commit(txnP, 0);
  if (rc != 0) {
    envP->err(envP, rc, "%s:%d Transaction commit failed.", __func__, __LINE__);
    goto failXit; 
  }

  goto cleanup;

failXit:
  if (txnP != NULL) {
    rc = txnP->abort(txnP);
    if (rc != 0) {
      envP->err(envP, rc, "Transaction abort failed.");
      goto failXit; 
    }
  }

  rc = 1;

cleanup:
  return rc;
}

int
get_portfolio(char *account_id, char *symbol, DB_TXN *txnP, DBC **cursorPP, DBT *key_ret, DBT *data_ret, BENCHMARK_DBS *benchmarkP) 
{
  DBC *cursorp = NULL;
  DB  *portfoliosdbP= NULL;
  DB  *portfoliossdbP= NULL;
  DB_ENV  *envP = NULL;
  DBT pkey, pdata;
  DBT key;
  int rc = 0;

  if (account_id == NULL || account_id[0] == '\0' || 
      symbol == NULL || symbol[0] == '\0' || 
      txnP == NULL || benchmarkP == NULL) 
  {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  portfoliosdbP = benchmarkP->portfolios_dbp;
  if (portfoliosdbP == NULL) {
    benchmark_error("Portfolios database is not open");
    goto failXit;
  }

  portfoliossdbP = benchmarkP->portfolios_sdbp;
  if (portfoliossdbP == NULL) {
    benchmark_error("Portfolios secondary database is not open");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&pkey, 0, sizeof(DBT));
  memset(&pdata, 0, sizeof(DBT));

  key.data = account_id;
  key.size = ID_SZ;

  rc = portfoliossdbP->cursor(portfoliossdbP, txnP,
                         &cursorp, 0);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to create cursor for Portfolios.", __FILE__, __LINE__, getpid());
    goto failXit;
  }
  
  while ((rc=cursorp->pget(cursorp, &key, &pkey, &pdata, DB_NEXT)) == 0)
  {
    /* TODO: Is it necessary this comparison? */
    if (strcmp(account_id, (char *)key.data) == 0) {
      if ( symbol != NULL && symbol[0] != '\0') {
        if (strcmp(symbol, ((PORTFOLIOS *)pdata.data)->symbol)) {
          rc = BENCHMARK_SUCCESS;
          goto cleanup;
        }
      }
    }
  }

failXit:
  rc = cursorp->close(cursorp);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to close cursor for Portfolios.", __FILE__, __LINE__, getpid());
  }
  cursorp = NULL;

  rc = BENCHMARK_FAIL;
  return rc;

cleanup:
  *cursorPP = cursorp;
  if (key_ret != NULL) {
    *key_ret = pkey;
  }

  if (data_ret != NULL) {
    *data_ret = pdata;
  }

  return rc;
}

int
get_stock(char *symbol, DB_TXN *txnP, DBC **cursorPP, DBT *key_ret, DBT *data_ret, int flags, BENCHMARK_DBS *benchmarkP) 
{
  DBC *cursorp = NULL;
  DB  *quotesdbP= NULL;
  DB_ENV  *envP = NULL;
  QUOTE   *quoteP = NULL;
  DBT key, data;
  int rc = 0;

  if (benchmarkP==NULL || txnP == NULL || cursorPP == NULL || symbol == NULL || symbol[0] == '\0')
  {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid argument");
    goto failXit;
  }

  quotesdbP = benchmarkP->quotes_dbp;
  if (quotesdbP == NULL) {
    benchmark_error("Quotes database is not open");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = symbol;
  key.size = (u_int32_t) strlen(symbol) + 1;
  rc = quotesdbP->cursor(quotesdbP, txnP, &cursorp, DB_READ_COMMITTED);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to create cursor for Quotes.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  /* Position the cursor */
  rc = cursorp->get(cursorp, &key, &data, DB_SET | DB_READ_COMMITTED | flags );
  if (rc == 0) {
    goto done;
  }
  envP->err(envP, rc, "[%s:%d] [%d] Failed to find record in Quotes.", __FILE__, __LINE__, getpid());

failXit:
  rc = cursorp->close(cursorp);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to close cursor for Quotes.", __FILE__, __LINE__, getpid());
  }
  cursorp = NULL;
  if (cursorPP != NULL) {
    *cursorPP = NULL;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  return BENCHMARK_FAIL;

done:
  if (cursorPP != NULL) {
    *cursorPP = cursorp;
  }

  if (key_ret != NULL) {
    *key_ret = key;
  }

  quoteP = data.data;
  benchmark_info("PID: %d, retrieved: %s $%f", getpid(), quoteP->symbol, quoteP->current_price);
  if (data_ret != NULL) {
    *data_ret = data;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  return BENCHMARK_SUCCESS;
}

static int
symbol_exists(char *symbol, DB_TXN *txnP, BENCHMARK_DBS *benchmarkP) 
{
  DBC *cursorp = NULL;
  DB  *stocksdbP = NULL;
  DB_ENV  *envP = NULL;
  DBT key, data;
  int exists = 0;
  int ret = 0;

  if (symbol == NULL || symbol[0] == '\0' || txnP == NULL || benchmarkP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  stocksdbP = benchmarkP->stocks_dbp;
  if (stocksdbP == NULL) {
    benchmark_error("Stocks database is not open");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = symbol;
  key.size = (u_int32_t) strlen(symbol) + 1;
  ret = stocksdbP->cursor(stocksdbP, txnP, &cursorp, DB_READ_COMMITTED);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to create cursor for Stocks.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  /* Position the cursor */
  ret = cursorp->get(cursorp, &key, &data, DB_SET);
  if (ret == 0) {
    exists = 1;
  }

  goto cleanup;

failXit:
cleanup:
  if (cursorp != NULL) {
      cursorp->close(cursorp);
  }

  return exists;
}

static int
account_exists(char *account_id, DB_TXN *txnP, BENCHMARK_DBS *benchmarkP) 
{
  DBC *cursorp = NULL;
  DB  *personaldbP = NULL;
  DB_ENV  *envP = NULL;
  DBT key, data;
  int exists = 0;
  int ret = 0;

  if (account_id == NULL || account_id[0] == '\0' || txnP == NULL || benchmarkP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }

  personaldbP = benchmarkP->personal_dbp;
  if (personaldbP == NULL) {
    benchmark_error("Personal database is not open");
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = account_id;
  key.size = (u_int32_t) strlen(account_id) + 1;
  ret = personaldbP->cursor(personaldbP, txnP, &cursorp, DB_READ_COMMITTED);
  if (ret != 0) {
    envP->err(envP, ret, "[%s:%d] [%d] Failed to create cursor for Personal.", __FILE__, __LINE__, getpid());
    goto failXit;
  }

  /* Position the cursor */
  ret = cursorp->get(cursorp, &key, &data, DB_SET);
  if (ret == 0) {
    exists = 1;
  }

  goto cleanup;

failXit:
cleanup:
  if (cursorp != NULL) {
      cursorp->close(cursorp);
  }

  return exists;
}

static int 
create_portfolio(char *account_id, char *symbol, float price, int amount, DB_TXN *txnP, BENCHMARK_DBS *benchmarkP)
{
  int rc = 0;
  PORTFOLIOS portfolio;
  DB_ENV  *envP = NULL;
  DBT key, data;

  envP = benchmarkP->envP;
  if (envP == NULL) {
    fprintf(stderr, "%s: Invalid arguments\n", __func__);
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  memset(&portfolio, 0, sizeof(PORTFOLIOS));
  sprintf(portfolio.portfolio_id, "%d", (rand()%500) + 100);
  sprintf(portfolio.account_id, "%s", account_id);
  sprintf(portfolio.symbol, "%s", symbol);
  portfolio.hold_stocks = 0;
  portfolio.to_sell = 0;
  portfolio.number_sell = 0;
  portfolio.price_sell = 0;
  portfolio.to_buy = 1;
  portfolio.number_buy = amount;
  portfolio.price_buy = price;

  /* Set up the database record's key */
  key.data = portfolio.portfolio_id;
  key.size = (u_int32_t)strlen(portfolio.portfolio_id) + 1;

  /* Set up the database record's data */
  data.data = &portfolio;
  data.size = sizeof(PORTFOLIOS);

#ifdef CHRONOS_DEBUG
  /* Put the data into the database */
  benchmark_debug(3,"Inserting: %s\n", (char *)key.data);
#endif

  rc = benchmarkP->portfolios_dbp->put(benchmarkP->portfolios_dbp, txnP, &key, &data, DB_NOOVERWRITE);
  if (rc != 0) {
    fprintf(stderr, "%s: Database put failed\n", __func__);
    goto failXit; 
  }

  goto cleanup;

failXit:
  rc = 1;

cleanup:
  return rc;
}

