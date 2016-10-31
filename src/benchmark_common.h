#ifndef _BENCHMARK_COMMON_H_
#define _BENCHMARK_COMMON_H_
/*
 * =====================================================================================
 *
 *       Filename:  benchmark_common.h
 *
 *    Description:  This file contains the definition of the tables that will be
 *                  used by the bechmarking application
 *
 *        Version:  1.0
 *        Created:  08/14/16 18:19:05
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricardo Zavaleta (rj.zavaleta@gmail.com)
 *   Organization:  CINVESTAV
 *
 * =====================================================================================
 */

#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "benchmark.h"

#define CHRONOS_INMEMORY
#define CHRONOS_SHMKEY 35
#define CHRONOS_PORTFOLIOS_NUM	100

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
extern char *optarg;
#define snprintf _snprintf
#else
#include <unistd.h>
#endif

#define MAXLINE   1024

#define PRIMARY_DB	0
#define SECONDARY_DB	1

/*
 * This benchmark is based on Kyoung-Don Kang et al. 
 * See the paper for reference.
 **/

/* These are the tables names in our app */
#define STOCKSDB          "Stocks"
#define QUOTESDB          "Quotes"
#define QUOTES_HISTDB     "Quotes_Hist"
#define PORTFOLIOSDB      "Portfolios"
#define PORTFOLIOSSECDB   "PortfoliosSec"
#define ACCOUNTSDB        "Accounts"
#define CURRENCIESDB      "Currencies"
#define PERSONALDB        "Personal"

/* Some of the tables above are catalogs. We have static
 * files that we use to populate them */
#define PERSONAL_FILE     "accounts.txt"
#define STOCKS_FILE       "companylist.txt"
#define CURRENCIES_FILE   "currencies.txt"
#define QUOTES_FILE       "quotes.txt"

#define STOCKS_FLAG       0x0001
#define PERSONAL_FLAG     0x0002
#define CURRENCIES_FLAG   0x0004
#define QUOTES_FLAG       0x0008
#define QUOTES_HIST_FLAG  0x0010
#define PORTFOLIOS_FLAG   0x0020
#define ACCOUNTS_FLAG     0x0040
#define ALL_DBS_FLAG      0x00FF

#define IS_STOCKS(_v)       (((_v) & STOCKS_FLAG) == STOCKS_FLAG)
#define IS_PERSONAL(_v)     (((_v) & PERSONAL_FLAG) == PERSONAL_FLAG)
#define IS_CURRENCIES(_v)   (((_v) & CURRENCIES_FLAG) == CURRENCIES_FLAG)
#define IS_QUOTES(_v)       (((_v) & QUOTES_FLAG) == QUOTES_FLAG)
#define IS_QUOTES_HIST(_v)  (((_v) & QUOTES_HIST_FLAG) == QUOTES_HIST_FLAG)
#define IS_PORTFOLIOS(_v)   (((_v) & PORTFOLIOS_FLAG) == PORTFOLIOS_FLAG)
#define IS_ACCOUNTS(_v)     (((_v) & ACCOUNTS_FLAG) == ACCOUNTS_FLAG)

/* Let's define our Benchmark DB, which translates to
 * multiple berkeley DBs*/
typedef struct benchmark_dbs {

  /* This is the environment */
  DB_ENV  *envP;

  /* These are the dbs */
  DB  *stocks_dbp;
  DB  *quotes_dbp;
  DB  *quotes_hist_dbp;
  DB  *portfolios_dbp;
  DB  *accounts_dbp;
  DB  *currencies_dbp;
  DB  *personal_dbp;

  /* secondary databases */
  DB  *portfolios_sdbp;

  /* Some other useful information */
  char *db_home_dir;

  /* Primary databases */
  char *stocks_db_name;
  char *quotes_db_name;
  char *quotes_hist_db_name;
  char *portfolios_db_name;
  char *accounts_db_name;
  char *currencies_db_name;
  char *personal_db_name;

  /* secondary databases */
  char *portfolios_sdb_name;

} BENCHMARK_DBS;

/* TODO: Are these constants useful? */
#define   ID_SZ           10
#define   NAME_SZ         128
#define   PWD_SZ          32
#define   USR_SZ          32
#define   LONG_NAME_SZ    200
#define   PHONE_SZ        16

/* Now let's define the structures for our tables */
/* TODO: Can we improve cache usage by modifying sizes? */

typedef struct stock {
  char              stock_symbol[ID_SZ];
  char              full_name[NAME_SZ];
} STOCK;

typedef struct quote {
  char      symbol[ID_SZ];
  float     current_price;
  char      trade_time[ID_SZ];
  float     low_price_day;
  float     high_price_day;
  float     perc_price_change;
  float     bidding_price;
  float     asking_price;
  long      trade_volume;
  char      market_cap[ID_SZ];
} QUOTE;

typedef struct quotes_hist {
  char      symbol[ID_SZ];
  int       current_price;
  time_t    trade_time;
  int       low_price_day;
  int       high_price_day;
  int       perc_price_change;
  int       bidding_price;
  int       asking_price;
  int       trade_volume;
  int       market_cap;
} QUOTES_HIST;

typedef struct portfolios {
  char      portfolio_id[ID_SZ];
  char      account_id[ID_SZ];
  char      symbol[ID_SZ];
  int       hold_stocks;
  char      to_sell;
  int       number_sell;
  int       price_sell;
  char      to_buy;
  int       number_buy;
  int       price_buy;
} PORTFOLIOS;

typedef struct account {
  char      account_id[ID_SZ];
  char      user_name[USR_SZ];
  char      password[PWD_SZ];
} ACCOUNT;

typedef struct currency {
  char      currency_symbol[ID_SZ];
  char      country[LONG_NAME_SZ];
  char      currency_name[NAME_SZ];
  int       exchange_rate_usd;
} CURRENCY;

typedef struct personal {
  char      account_id[ID_SZ];
  char      last_name[NAME_SZ];
  char      first_name[NAME_SZ];
  char      address[NAME_SZ];
  char      address_2[NAME_SZ];
  char      city[NAME_SZ];
  char      state[NAME_SZ];
  char      country[NAME_SZ];
  char      phone[PHONE_SZ];
  char      email[LONG_NAME_SZ];
} PERSONAL;

extern char *symbolsArr[];

/* Function prototypes */
int	databases_setup(BENCHMARK_DBS *, int, const char *, FILE *);
int	databases_open(DB **, const char *, const char *, FILE *, int);
int	databases_close(BENCHMARK_DBS *);

void	initialize_benchmarkdbs(BENCHMARK_DBS *);
void	set_db_filenames(BENCHMARK_DBS *my_stock);

int 
show_portfolios(BENCHMARK_DBS *benchmarkP);

int 
place_order(int account, char *symbol, float price, int amount, BENCHMARK_DBS *benchmarkP);

int 
sell_stocks(int account, char *symbol, float price, int amount, BENCHMARK_DBS *benchmarkP);

#endif
