#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "chronos_config.h"
#include "benchmark_common.h"

int benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

static int
usage()
{
    static const char usage[] = "This script implements the purchase of a given symbol for one or all users. \n"
                                "Using -a, the purchase is applied to all users in the system. Otherwise, the \n"
                                "operation is performed for the specified user and symbol \n\n"
                                "If no symbol is provided, then the symbol is chosen randomly among  the list \n"
                                "of available symbols.\n\n"
                                "For the operation, user can specify the price to sell the stock (-p option)  \n"
                                "and the amount of stocks (-c option).\n\n"
                                "Finally, an initial load can be specified with the option -l.\n\n"
                                ;
    fprintf(stderr, "%s", usage);
    fprintf(stderr, "purchase_stock\n\n");
    fprintf(stderr, "\t-a \t\tApply to all users\n");
    fprintf(stderr, "\t-s \t\tSymbol\n");
    fprintf(stderr, "\t-u \t\tUser Id\n");
    fprintf(stderr, "\t-p \t\tPrice\n");
    fprintf(stderr, "\t-c \t\tAmount\n");
    fprintf(stderr, "\t-l \t\tPerform an initial load\n");
    fprintf(stderr, "\t-h \t\tThis help\n");
    fprintf(stderr, "\n");
    return (-1);
}

/* 
 * This program queries the tables in chronos  
 */

int main(int argc, char *argv[]) 
{
  int i;
  int ch;
  int doInitialLoad = 0;
  int price = -1;
  int amount = -1;
  int apply_to_all = 0;
  int symbol_to_purchase = -1;
  int account_to_purchase = -1;
  int rc = BENCHMARK_SUCCESS;
  void *benchmarkCtxtP = NULL;

  srand(time(NULL));

  while ((ch = getopt(argc, argv, "as:u:p:c:lh")) != EOF)
  {
    switch (ch) {
      case 'a':
        apply_to_all = 1;
        break;

      case 's':
        symbol_to_purchase = atoi(optarg);
        break;

      case 'u':
        account_to_purchase = atoi(optarg);
        break;

      case 'p':
        price = atoi(optarg);
        break;

      case 'c':
        amount = atoi(optarg);
        break;

      case 'l':
        doInitialLoad = 1;
        break;

      case 'h':

      default:
          return (usage());
    }
  }
  
  if (doInitialLoad) {
    if (benchmark_initial_load(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to perform initial load");
      goto failXit;
    }
  }

  if (benchmark_handle_alloc(&benchmarkCtxtP, 0, CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to perform initial load");
    goto failXit;
  }

  benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN + 5;
  if (doInitialLoad) {
    if (benchmark_load_portfolio(benchmarkCtxtP) != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to load portfolios");
      goto failXit;
    }
  }

  if (apply_to_all) {
    for (i=1; i<BENCHMARK_NUM_ACCOUNTS; i++) {
      if (symbol_to_purchase < 0)
        symbol_to_purchase = rand() % BENCHMARK_NUM_SYMBOLS;

      account_to_purchase = i;

      if (benchmark_purchase(account_to_purchase,
                         symbol_to_purchase,
                         price,
                         amount,
                         1,     /* force_apply */
                         benchmarkCtxtP, NULL) != BENCHMARK_SUCCESS) {
        benchmark_error("Failed to purchase stocks");
        continue;
      }
    }
  }
  else {
    assert(0 <= symbol_to_purchase && symbol_to_purchase < BENCHMARK_NUM_SYMBOLS);
    assert(1 <= account_to_purchase && account_to_purchase < BENCHMARK_NUM_ACCOUNTS + 1);

    if (benchmark_purchase(account_to_purchase,
                       symbol_to_purchase,
                       price,
                       amount,
                       1,     /* force_apply */
                       benchmarkCtxtP, NULL) != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to purchase stocks");
    }
  }
  benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

  goto cleanup;

failXit:
  rc = BENCHMARK_FAIL;

cleanup:
  if (benchmark_handle_free(benchmarkCtxtP) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to free handle");
    goto failXit;
  }

  return rc;
}

