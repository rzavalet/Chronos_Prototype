#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "chronos_config.h"
#include "benchmark_common.h"

int benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

static int
usage()
{
    fprintf(stderr, "query_stocks\n");
    fprintf(stderr, "\t-l \t\tPerform an initial load\n");
    fprintf(stderr, "\t-a \t\tQuery all records\n");
    fprintf(stderr, "\t-c \t\tNumber of portfolios to show\n");
    fprintf(stderr, "\t-u \t\tOnly show the user information\n");
    fprintf(stderr, "\t-p \t\tOnly dump the portfolios information\n");
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
  int showAll = 0;
  int onlyUsers = 0;
  int onlyPortfolios = 0;
  int num_reps = 100;
  int data_item = 0;
  int rc = BENCHMARK_SUCCESS;
  void *benchmarkCtxtP = NULL;

  srand(time(NULL));

  while ((ch = getopt(argc, argv, "paulc:h")) != EOF)
  {
    switch (ch) {
      case 'a':
        showAll = 1;
        break;

      case 'u':
        onlyUsers = 1;
        break;

      case 'l':
        doInitialLoad = 1;
        break;

      case 'p':
        onlyPortfolios = 1;
        break;

      case 'c':
        num_reps = atoi(optarg);
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
    if (show_portfolios(NULL, onlyUsers, benchmarkCtxtP) != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to retrieve all portfolios");
      goto failXit;
    }
  }
  else if (onlyPortfolios) {
    if (show_all_portfolios(benchmarkCtxtP) != BENCHMARK_SUCCESS) {
      benchmark_error("Failed to retrieve all portfolios");
      goto failXit;
    }
  }
  else {
    for (i=0; i<num_reps; i++) {
      char user_id[ID_SZ];
      snprintf(user_id, sizeof(user_id), "%d", rand() % 50 + 1);
      if (show_portfolios(user_id, onlyUsers, benchmarkCtxtP) != BENCHMARK_SUCCESS) {
        benchmark_error("Failed to retrieve all portfolios");
        goto failXit;
      }
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

