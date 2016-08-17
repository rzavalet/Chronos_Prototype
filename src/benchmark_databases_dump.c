/*
 * =====================================================================================
 *
 *       Filename: benchmark_databases_dump.c 
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
int usage(void);

int show_personal_item(void *);
int show_currencies_item(void *);

int show_personal_records(BENCHMARK_DBS *);
int show_currencies_records(BENCHMARK_DBS *);

int
usage()
{
    fprintf(stderr, "benchmark_databases_dump ");
    fprintf(stderr, " [-h <database home>]\n");
    return (-1);
}

int
main(int argc, char *argv[])
{
  BENCHMARK_DBS my_benchmark;
  int ch, ret;
  char *itemname;
  int which_tables = 0;

  /* Initialize the BENCHMARK_DBS struct */
  initialize_benchmarkdbs(&my_benchmark);

  /* Parse the command line arguments */
  itemname = NULL;
  while ((ch = getopt(argc, argv, "h:d:?")) != EOF)
  {
    switch (ch) {
      case 'h':
        if (optarg[strlen(optarg)-1] != '/' &&
          optarg[strlen(optarg)-1] != '\\') 
        {
          return (usage());
        }
        my_benchmark.db_home_dir = optarg;
        break;

      case 'd':
        if (strcmp(optarg, "STOCKSDB") == 0 ) {
          which_tables |= STOCKS_FLAG;
        }
        else if (strcmp(optarg, "PERSONALDB") == 0 ) {
          which_tables |= PERSONAL_FLAG;
        }
        else if (strcmp(optarg, "CURRENCIESDB") == 0 ) {
          which_tables |= CURRENCIES_FLAG;
        }

        break;

      case '?':

      default:
          return (usage());
    }
  }

  /* Identify the files that hold our databases */
  set_db_filenames(&my_benchmark);

  /* Open all databases */
  ret = databases_setup(&my_benchmark, ALL_DBS_FLAG, "benchmark_databases_dump", stderr);
  if (ret != 0) {
    fprintf(stderr, "Error opening databases\n");
    databases_close(&my_benchmark);
    return (ret);
  }

  if ((which_tables & STOCKS_FLAG) == STOCKS_FLAG)
	  ret = show_stocks_records(&my_benchmark);

  if ((which_tables & PERSONAL_FLAG) == PERSONAL_FLAG)
    ret = show_personal_records(&my_benchmark);

  if ((which_tables & CURRENCIES_FLAG) == CURRENCIES_FLAG)
    ret = show_currencies_records(&my_benchmark);

  /* close our databases */
  databases_close(&my_benchmark);
  return (ret);
}


int show_personal_records(BENCHMARK_DBS *my_benchmarkP)
{
  DBC *personal_cursorp;
  DBT key, data;
  char *personal_element;
  int exit_value, ret;

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  printf("================= SHOWING PERSONAL DATABASE ==============\n");

  my_benchmarkP->personal_dbp->cursor(my_benchmarkP->personal_dbp, NULL,
                                    &personal_cursorp, 0);

  exit_value = 0;
  while ((ret =
    personal_cursorp->get(personal_cursorp, &key, &data, DB_NEXT)) == 0)
  {
    (void) show_personal_item(data.data);
  }

  personal_cursorp->close(personal_cursorp);
  return (exit_value);
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


int show_currencies_records(BENCHMARK_DBS *my_benchmarkP)
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




