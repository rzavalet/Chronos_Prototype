/*===================================================================
 * This file contains the required configuration for the 
 * chronos server
 *===================================================================*/
#ifndef _CHRONOS_CONFIG_H_
#define _CHRONOS_CONFIG_H_

/* These are the directories where the databases and the datafiles live.
 * Before starting up the server, the datafiles should be moved to the
 * specified directory. The datafiles are used to populate the Chronos
 * tables */
#define CHRONOS_SERVER_HOME_DIR       "/tmp/chronos/databases"
#define CHRONOS_SERVER_DATAFILES_DIR  "/tmp/chronos/datafiles"

/* By default the Chronos server runs in port 5000 */
#define CHRONOS_SERVER_ADDRESS  "127.0.0.1"
#define CHRONOS_SERVER_PORT     5000

/* In the Chronos paper, the number of client threads start
 * at 900 and it can increase up to 1800
 */
#define CHRONOS_NUM_CLIENT_THREADS    900

/* In the Chronos paper, the number of server threads
 * is 350 for their linux settings
 */
#ifdef CHRONOS_DEBUG
#define CHRONOS_NUM_SERVER_THREADS    5
#else
#define CHRONOS_NUM_SERVER_THREADS    350
#endif

/* By default, updates to the quotes table is performed
 * by 100 threads
 */
#ifdef CHRONOS_DEBUG
#define CHRONOS_NUM_UPDATE_THREADS    1
#else
#define CHRONOS_NUM_UPDATE_THREADS    100
#endif

/* Chronos server has two ready queues. The default size of them is 1024 */
#ifdef CHRONOS_DEBUG
#define CHRONOS_READY_QUEUE_SIZE     (10)
#else
#define CHRONOS_READY_QUEUE_SIZE     (1024)
#endif

/* The update period is initially set to 0.5 in Chronos
 */
#ifdef CHRONOS_DEBUG
#define CHRONOS_INITIAL_UPDATE_PERIOD_MS  5000
#else
#define CHRONOS_INITIAL_UPDATE_PERIOD_MS  500
#endif

/* Chronos uses the following relation for the flexible
 * validity interval
 */
#define CHRONOS_UPDATE_PERIOD_MS_TO_FVI(_p)     (2.0 * (_p))
#define CHRONOS_FVI_TO_UPDATE_PERIDO_MS(_fvi)   (0.5 * (_fvi))

/* By default, Chronos uses \beta=2
 */
#define CHRONOS_UPDATE_PERIOD_RELAXATION_BOUND  2

/* Each update thread handles 30 stocks
 */
#ifdef CHRONOS_DEBUG
#define CHRONOS_NUM_STOCK_UPDATES_PER_UPDATE_THREAD  5
#else
#define CHRONOS_NUM_STOCK_UPDATES_PER_UPDATE_THREAD  300
#endif

/* Chronos tries to relax the period by 10% every time
 */
#define CHRONOS_FVI_RELAXATION_AMOUNT   10.0
#define CHRONOS_FVI_RELAX(_fvi)         ((_fvi) * (1.0 + CHRONOS_FVI_RELAXATION_AMOUNT / 100.0))

/* Some utility macros to perform conversion of time units */
#define CHRONOS_MS_TO_S(_ms)            ((_ms) / 1000.0)
#define CHRONOS_S_TO_MS(_s)             ((_s) * 1000.0)

#define CHRONOS_MIN_TO_S(_m)            ((_m) * 60.0)
#define CHRONOS_S_TO_MIN(_s)            ((_s) / 60.0)

/* The sampling in Chronos is done every 30 seconds
 */
#define CHRONOS_SAMPLING_PERIOD_SEC     (30)

/* Chronos experiments take 15 minutes
 */
#ifdef CHRONOS_DEBUG
#define CHRONOS_EXPERIMENT_DURATION_SEC (CHRONOS_MIN_TO_S(1))
#else
#define CHRONOS_EXPERIMENT_DURATION_SEC (CHRONOS_MIN_TO_S(15))
#endif

#endif
