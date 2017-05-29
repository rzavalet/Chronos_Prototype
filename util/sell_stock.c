#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "chronos_config.h"
#include "benchmark_common.h"

int benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

static int
usage()
{
    fprintf(stderr, "sell_stock\n");
    fprintf(stderr, "\t-l \t\tPerform an initial load\n");
    fprintf(stderr, "\t-a \t\tQuery all records\n");
    fprintf(stderr, "\t-h \t\tThis help\n");
    fprintf(stderr, "\n");
    return (-1);
}

/* 
 * This program queries the tables in chronos  
 */

int main(int argc, char *argv[]) 
{
  int ch;
  int doInitialLoad = 0;
  int showAll = 0;
  int data_item = 0;
  int rc = BENCHMARK_SUCCESS;
  void *benchmarkCtxtP = NULL;

  srand(time(NULL));

  while ((ch = getopt(argc, argv, "alh")) != EOF)
  {
    switch (ch) {
      case 'a':
        showAll = 1;
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

  if (showAll) {
  }
  else {
    if (benchmark_sell(benchmarkCtxtP, NULL) != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to sell stocks");
      goto failXit;
    }
  }
  benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

  benchmark_info("** Retrieved info for: %d", data_item);
  
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


