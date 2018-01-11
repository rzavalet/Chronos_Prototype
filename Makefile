#TODO: Make this more build independent. Eg DEF_LIB, CFLAGS, SRCDIR and BINDIR

SRCDIR= ./src
BINDIR= ./bin
UTILSDIR= ./util
INCLUDEDIR= ./include
HOMEDIR= ./databases
DATAFILESDIR= ./datafiles
TESTDIR= ./test

##################################################
# General library information.
##################################################
DEF_LIB=	-L/usr/local/BerkeleyDB.6.2/lib -ldb-6.2 -lrt
LIBTOOL=	./libtool

POSTLINK=	$(LIBTOOL) --mode=execute true
#SOLINK=		$(LIBTOOL) --mode=link cc -avoid-version -O3 
SOLINK=		$(LIBTOOL) --mode=link cc -avoid-version -O0

##################################################
# C API.
##################################################

#----- With optimization ----------------
#CFLAGS=		-c -I/usr/local/BerkeleyDB.6.2/include -O3 -g
#CCLINK=		$(LIBTOOL) --mode=link cc -O3 
#LDFLAGS=

#----- With profiler ----------------
#CFLAGS=		-c -I$(INCLUDEDIR) -I/usr/local/BerkeleyDB.6.2/include -pg -g -Wall
#CCLINK=		$(LIBTOOL) --mode=link cc
#LDFLAGS= -pg

#----- With debugger ----------------
#CFLAGS=		-c -I$(INCLUDEDIR) -I/usr/local/BerkeleyDB.6.2/include -g -Wall -DCHRONOS_DEBUG -DCHRONOS_UPDATE_TRANSACTIONS_ENABLED -DBENCHMARK_DEBUG
#CFLAGS=		-c -I$(INCLUDEDIR) -I/usr/local/BerkeleyDB.6.2/include -g -Wall -DCHRONOS_DEBUG -DCHRONOS_USER_TRANSACTIONS_ENABLED -DBENCHMARK_DEBUG
#CFLAGS=		-c -I$(INCLUDEDIR) -I/usr/local/BerkeleyDB.6.2/include -g -Wall -DCHRONOS_DEBUG -DCHRONOS_USER_TRANSACTIONS_ENABLED -DCHRONOS_UPDATE_TRANSACTIONS_ENABLED -DBENCHMARK_DEBUG
CFLAGS=		-c -I$(INCLUDEDIR) -I/usr/local/BerkeleyDB.6.2/include -g -Wall -DCHRONOS_DEBUG -DCHRONOS_USER_TRANSACTIONS_ENABLED -DBENCHMARK_DEBUG -DCHRONOS_ALL_TXN_AVAILABLE -DCHRONOS_UPDATE_TRANSACTIONS_ENABLED
#-DCHRONOS_SAMPLING_ENABLED 
CCLINK=		$(LIBTOOL) --mode=link cc
LDFLAGS=


CC=		$(LIBTOOL) --mode=compile cc
LIBS=		 -lpthread

##################################################
# Targets 
##################################################
all: startup_server startup_client 

##################################################
# Compile and link
##################################################
OBJECTS = benchmark_common.lo benchmark_initial_load.lo benchmark_stocks.lo populate_portfolios.lo refresh_quotes.lo \
					view_stock_txn.lo view_portfolio_txn.lo purchase_txn.lo sell_txn.lo chronos_queue.lo \
					chronos_client.lo chronos_packets.lo chronos_cache.lo chronos_environment.lo

benchmark_common.lo: $(SRCDIR)/benchmark_common.c
	$(CC) $(CFLAGS) $?

benchmark_initial_load.lo: $(SRCDIR)/benchmark_initial_load.c
	$(CC) $(CFLAGS) $?

benchmark_stocks.lo: $(SRCDIR)/benchmark_stocks.c
	$(CC) $(CFLAGS) $?

populate_portfolios.lo:	$(SRCDIR)/populate_portfolios.c 
	$(CC) $(CFLAGS) $?

refresh_quotes.lo:	$(SRCDIR)/refresh_quotes.c
	$(CC) $(CFLAGS) $?

view_stock_txn.lo:	$(SRCDIR)/view_stock_txn.c
	$(CC) $(CFLAGS) $?

view_portfolio_txn.lo:	$(SRCDIR)/view_portfolio_txn.c
	$(CC) $(CFLAGS) $?

purchase_txn.lo:	$(SRCDIR)/purchase_txn.c
	$(CC) $(CFLAGS) $?

sell_txn.lo:	$(SRCDIR)/sell_txn.c
	$(CC) $(CFLAGS) $?

chronos_queue.lo:	$(SRCDIR)/chronos_queue.c
	$(CC) $(CFLAGS) $?

chronos_client.lo:	$(SRCDIR)/chronos_client.c
	$(CC) $(CFLAGS) $?

chronos_packets.lo: $(SRCDIR)/chronos_packets.c
	$(CC) $(CFLAGS) $?

chronos_cache.lo: $(SRCDIR)/chronos_cache.c
	$(CC) $(CFLAGS) $?

chronos_environment.lo: $(SRCDIR)/chronos_environment.c
	$(CC) $(CFLAGS) $?

##################################################
# Build the server
##################################################
startup_server.lo :	$(SRCDIR)/startup_server.c
	$(CC) $(CFLAGS) $?

startup_server : startup_server.lo $(OBJECTS) 
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) startup_server.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@

##################################################
# Build the client
##################################################
startup_client.lo :	$(SRCDIR)/startup_client.c
	$(CC) $(CFLAGS) $?

startup_client : startup_client.lo $(OBJECTS)
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) startup_client.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@

##################################################
# Useful targets for running the benchmark
##################################################
init :
	-@echo "#---------------------------------------------------#"
	-@echo "#--------- Setting up directory structure ----------#"
	-@echo "#---------------------------------------------------#"
	-rm -rf /tmp/cnt*
	-rm -rf /tmp/upd*
	-rm -rf /tmp/chronos/databases
	-rm -rf /tmp/chronos/datafiles
	mkdir -p /tmp/chronos/databases
	mkdir -p /tmp/chronos/datafiles
	cp datafiles/* /tmp/chronos/datafiles
	-@echo "#-------------------- DONE -------------------------#"

cscope:
	cscope -bqRv

.PHONY : clean

clean :
	-rm *.o
	-rm *.lo
	-rm -rf bin
