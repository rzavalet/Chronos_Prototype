/*
 * =====================================================================================
 *
 *       Filename:  view_stock_txn.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/16/16 23:33:23
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ricardo Zavaleta (), rj.zavaleta@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "benchmark_common.h"

/* Forward declarations */
int usage(void);

int
usage()
{
  fprintf(stderr, "view_stock_txn ");
  fprintf(stderr, " [-h <database_home_directory>]\n");
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
  while ((ch = getopt(argc, argv, "h:?")) != EOF)
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

        break;

      case '?':

      default:
          return (usage());
    }
  }

  /* Identify the files that hold our databases */
  set_db_filenames(&my_benchmark);

  /* Open stocks database */
  ret = databases_setup(&my_benchmark, STOCKS_FLAG, "benchmark_databases_dump", stderr);
  if (ret != 0) {
    fprintf(stderr, "Error opening databases\n");
    databases_close(&my_benchmark);
    return (ret);
  }

	ret = show_stocks_records(&my_benchmark);

  /* close our databases */
  databases_close(&my_benchmark);
  return (ret);
}
