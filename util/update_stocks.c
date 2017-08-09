
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "chronos_config.h"
#include "benchmark_common.h"

int benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

static int
usage()
{
    fprintf(stderr, "update_stocks\n");
    fprintf(stderr, "\t-l \t\tPerform an initial load\n");
    fprintf(stderr, "\t-c \t\tNumber of updates in this program run\n");
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
  int updateAll = 0;
  int updateValue = 0;
  int num_reps = 100;                  /* By default, we update 100 rows per program execution */
  void *benchmarkCtxtP = NULL;
  int attemptedTxns = 0;
  int failedTxns = 0;
  int successfulTxns = 0;

  srand(time(NULL));

  while ((ch = getopt(argc, argv, "lhav:c:")) != EOF)
  {
    switch (ch) {
      case 'a':
        updateAll = 1;
        break;

      case 'v':
        updateValue = atoi(optarg);
        break;

      case 'c':
        num_reps = atoi(optarg);
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

  //benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN + 5;
  if (updateAll) {
    /* If the updateAll flag is set, then we update all available symbols.
     * The update value has to be received from the command line. Otherwise
     * the update value is gonna be 0.
     */
    int symbolToUpdate;
    for (i=0; i<BENCHMARK_NUM_SYMBOLS; i++) {
      attemptedTxns ++;
      symbolToUpdate = i;                             /* The symbol to update correspond to the for-loop idx */
      if (benchmark_refresh_quotes(benchmarkCtxtP, &symbolToUpdate, updateValue) != BENCHMARK_SUCCESS) {
        benchmark_error("Failed to update stock info");
        failedTxns ++;                                /* Simply try another transaction */
        continue;
      }
      successfulTxns ++;
    }
  }
  else {
    /* If the updateAll flag is not set, then we update a randomly chosen row.
     * The update value is simply the index of the current iteration.
     */
    for (i=0; i<num_reps; i++) {
      attemptedTxns ++;
      if (benchmark_refresh_quotes(benchmarkCtxtP, NULL, i) != BENCHMARK_SUCCESS) {
        benchmark_error("Failed to update stock info");
        /* For now, we don't care if the transaction failed. We simply
	 * try another transaction right away. */
        failedTxns ++;
        continue;
      }
      successfulTxns ++;
    }
  }
  //benchmark_debug_level = BENCHMARK_DEBUG_LEVEL_MIN;

  if (benchmark_handle_free(benchmarkCtxtP) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to free handle");
    goto failXit;
  }

  benchmark_info("Attempted transactions: %d, Successful transactions: %d, Failed transactions: %d", attemptedTxns, successfulTxns, failedTxns);
  assert(attemptedTxns == successfulTxns+failedTxns);

  return 0;

failXit:
  if (benchmark_handle_free(benchmarkCtxtP) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to free handle");
  }

  return 1;
}

