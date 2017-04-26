#include "chronos_config.h"
#include "benchmark.h"

/* 
 * This program queries the tables in chronos  
 */

int main(int argc, char *argv[]) 
{

  int txn_rc = 0;
  void *benchmarkCtxtP = NULL;

  if (benchmark_initial_load(CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to perform initial load");
    goto failXit;
  }

  if (benchmark_handle_alloc(&benchmarkCtxtP, CHRONOS_SERVER_HOME_DIR, CHRONOS_SERVER_DATAFILES_DIR) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to perform initial load");
    goto failXit;
  }

  if (benchmark_view_stock(benchmarkCtxtP, &data_item) != BENCHMARK_SUCCESS) {
    benchmark_error("Failed to retrieve stock info");
    goto failXit;
  }

  if (benchmark_handle_free(benchmarkCtxtP) != CHRONOS_SUCCESS) {
    benchmark_error("Failed to free handle");
    goto failXit;
  }

  return 0;

failXit:
  return 1;
}

