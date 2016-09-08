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

static int
get_account_id(DB *sdbp,          /* secondary db handle */
              const DBT *pkey,   /* primary db record's key */
              const DBT *pdata,  /* primary db record's data */
              DBT *skey);         /* secondary db record's key */

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
#ifdef CHRONOS_DEBUG
  printf("Symbol: %s\n", symbol);
  printf("\tName: %s\n", name);
#endif

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

  /*
   * If this is a secondary database, then we want to allow
   * sorted duplicates.
   */
  if (is_secondary) {
    ret = dbp->set_flags(dbp, DB_DUPSORT);
    if (ret != 0) {
        envP->err(envP, ret, "Attempt to set DUPSORT flags failed.",
          file_name);
        return (ret);
    }
  }

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

  if (benchmarkP == NULL) {
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
      fprintf(stderr, "Error setting lock detect: %s\n",
          db_strerror(rc));
      goto failXit;
  } 

  rc = envP->set_shm_key(envP, CHRONOS_SHMKEY); 
  if (rc != 0) {
      fprintf(stderr, "Error setting shm key: %s\n",
          db_strerror(rc));
      goto failXit;
  } 
#ifdef CHRONOS_DEBUG
  printf("shm key set to: %d\n", CHRONOS_SHMKEY); 
#endif
#endif

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

    ret = open_database(benchmarkP->envP,
                        &(benchmarkP->portfolios_sdbp),
                        benchmarkP->portfolios_sdb_name,
                        program_name, error_fileP,
                        SECONDARY_DB);
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
  printf("databases opened successfully\n");
#endif
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
  memset(benchmarkP, 0, sizeof(*benchmarkP));
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

#ifdef CHRONOS_DEBUG
  printf("databases closed.\n");
#endif
  return (0);
}


int 
show_stocks_records(char *symbolId, BENCHMARK_DBS *benchmarkP)
{
  DBC *stock_cursorp;
  DBT key, data;
  int exit_value, ret;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  if (symbolId != NULL && symbolId[0] != '\0') {
    key.data = symbolId;
    key.size = (u_int32_t)strlen(symbolId) + 1;
  }

  benchmarkP->stocks_dbp->cursor(benchmarkP->stocks_dbp, NULL,
                                    &stock_cursorp, 0);

  exit_value = 0;
  while ((ret =
    stock_cursorp->get(stock_cursorp, &key, &data, DB_NEXT)) == 0)
  {
    /*Uhhh, this is ugly.... */
    if (strcmp(symbolId, (char *)key.data) == 0) {
      (void) show_stock_item(data.data);
    }
  }

  stock_cursorp->close(stock_cursorp);
  return (exit_value);
}

int 
show_portfolios(BENCHMARK_DBS *benchmarkP)
{
  DBC *personal_cursorP;
  DBT key, data;
  int rc = 0;
  int numClients = 0;

  if (benchmarkP == NULL || benchmarkP->personal_dbp == NULL) {
    fprintf(stderr, "%s: Invalid argument\n", __func__);
    goto failXit;
  }

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  /* First read the personall account */
  benchmarkP->personal_dbp->cursor(benchmarkP->personal_dbp, NULL,
                                    &personal_cursorP, 0);

  while ((rc=personal_cursorP->get(personal_cursorP, &key, &data, DB_NEXT)) == 0)
  {
#ifdef CHRONOS_DEBUG
     printf("================= SHOWING PORTFOLIO ==============\n");
#endif
    (void) show_personal_item(data.data);   
    (void) show_one_portfolio(key.data, benchmarkP);
    numClients ++;
#ifdef CHRONOS_DEBUG
     printf("==================================================\n");
#endif
  }

  personal_cursorP->close(personal_cursorP);

#ifdef CHRONOS_DEBUG
  printf("Displayed info about %d clients\n", numClients);
#endif
  return (rc);

failXit:
  rc = 1;
  return rc;
}

int
show_one_portfolio(char *account_id, BENCHMARK_DBS *benchmarkP)
{
  DBC *portfolio_cursorP;
  DBT key;
  DBT pkey, pdata;
  char *symbolIdP = NULL;
  int rc = 0;
  int numPortfolios = 0;

  memset(&key, 0, sizeof(DBT));
  memset(&pkey, 0, sizeof(DBT));
  memset(&pdata, 0, sizeof(DBT));

  key.data = account_id;
  key.size = ID_SZ;
  
  benchmarkP->portfolios_sdbp->cursor(benchmarkP->portfolios_sdbp, NULL, 
                                   &portfolio_cursorP, 0);
  
  while ((rc=portfolio_cursorP->pget(portfolio_cursorP, &key, &pkey, &pdata, DB_NEXT)) == 0)
  {
    (void) show_portfolio_item(pdata.data, &symbolIdP);
    if (symbolIdP != NULL && symbolIdP[0] != '\0') {
      (void) show_stocks_records(symbolIdP, benchmarkP);
    }
    numPortfolios ++;
  }

  portfolio_cursorP->close(portfolio_cursorP);
#ifdef CHRONOS_DEBUG
  printf("Displayed info about %d portfolios\n", numPortfolios);
#endif
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
  printf("AccountId: %s\n", account_id);
  printf("\tLast Name: %s\n", last_name);
  printf("\tFirst Name: %s\n", first_name);
  printf("\tAddress: %s\n", address);
  printf("\tAdress 2: %s\n", address_2);
  printf("\tCity: %s\n", city);
  printf("\tState: %s\n", state);
  printf("\tCountry: %s\n", country);
  printf("\tPhone: %s\n", phone);
  printf("\tEmail: %s\n", email);

  return 0;
}


int 
show_currencies_records(BENCHMARK_DBS *my_benchmarkP)
{
  DBC *currencies_cursorp;
  DBT key, data;
  char *currencies_element;
  int exit_value, ret;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  printf("================= SHOWING CURRENCIES DATABASE ==============\n");

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

int
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
  printf("Currency Symbol: %s\n", currency_symbol);
  printf("\tExchange Rate: %d\n", exchange_rate_usd);
  printf("\tCountry: %s\n", country);
  printf("\tName: %s\n", currency_name);

  return 0;
}

int
show_portfolio_item(void *vBuf, char **symbolIdPP)
{
  char      *portfolio_id;
  char      *account_id;
  char      *symbol;
  int       hold_stocks;
  char      to_sell;
  int       number_sell;
  int       price_sell;
  char      to_buy;
  int       number_buy;
  int       price_buy;

  size_t buf_pos = 0;
  char *buf = (char *)vBuf;

  portfolio_id = buf;
  buf_pos += ID_SZ;
  
  account_id = buf + buf_pos;
  buf_pos += ID_SZ;

  symbol = buf + buf_pos;
  buf_pos += ID_SZ;

  hold_stocks = *((int *)(buf + buf_pos));
  buf_pos += sizeof(int);

  to_sell = *((char *)(buf + buf_pos));
  buf_pos += sizeof(char);

  number_sell = *((int *)(buf + buf_pos));
  buf_pos += sizeof(int);

  price_sell = *((int *)(buf + buf_pos));
  buf_pos += sizeof(int);

  to_buy = *((char *)(buf + buf_pos));
  buf_pos += sizeof(char);

  number_buy = *((int *)(buf + buf_pos));
  buf_pos += sizeof(int);

  price_buy = *((int *)(buf + buf_pos));
 
  /* Display all this information */
  printf("Portfolio ID: %s\n", portfolio_id);
  printf("\tAccount ID: %s\n", account_id);
  printf("\tSymbol ID: %s\n", symbol);
  printf("\t# Stocks Hold: %d\n", hold_stocks);
  printf("\tSell?: %d\n", to_sell);
  printf("\t# Stocks to sell: %d\n", number_sell);
  printf("\tPrice to sell: %d\n", price_sell);
  printf("\tBuy?: %d\n", to_buy);
  printf("\t# Stocks to buy: %d\n", number_buy);
  printf("\tPrice to buy: %d\n", price_buy);

  if (symbolIdPP) {
    *symbolIdPP = symbol; 
  }

  return 0;
}
