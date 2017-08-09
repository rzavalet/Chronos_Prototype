#include "benchmark_common.h"
#include "benchmark_stocks.h"

int
benchmark_stocks_stats_get(BENCHMARK_DBS *benchmarkP)
{
  int             rc = BENCHMARK_SUCCESS;
  DB             *quotes_dbp = NULL;
  DB_BTREE_STAT  *quotes_statsP = NULL;
  DB_ENV         *envP = NULL;

  if (benchmarkP == NULL) {
    goto failXit;
  }

  BENCHMARK_CHECK_MAGIC(benchmarkP);

  envP = benchmarkP->envP;
  if (envP == NULL) {
    benchmark_error("Invalid arguments");
    goto failXit;
  }
  quotes_dbp = benchmarkP->quotes_dbp;
  if (quotes_dbp == NULL) {
    benchmark_error("Quotes table is uninitialized");
    goto failXit;
  }

  rc = quotes_dbp->stat(quotes_dbp, NULL, (void *)&quotes_statsP, 0 /* no FAST_STAT */);
  if (rc != 0) {
    envP->err(envP, rc, "[%s:%d] [%d] Failed to obtain stats from Quotes table.", __FILE__, __LINE__, getpid());
    goto failXit; 
  }

  benchmarkP->number_stocks = quotes_statsP->bt_nkeys;
  benchmark_debug(5, "Number of keys in Quotes table is: %d", benchmarkP->number_stocks);

  BENCHMARK_CHECK_MAGIC(benchmarkP);
  goto cleanup;

failXit:
  BENCHMARK_CHECK_MAGIC(benchmarkP);
  rc = BENCHMARK_FAIL;

cleanup:
  if (quotes_statsP) {
    free(quotes_statsP);
  }

  return rc;
}



