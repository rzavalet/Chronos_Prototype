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
#CFLAGS=		-c -I/usr/local/BerkeleyDB.6.2/include -O3 -g
CFLAGS=		-c -I$(INCLUDEDIR) -I/usr/local/BerkeleyDB.6.2/include -g -Wall
CC=		$(LIBTOOL) --mode=compile cc
#CCLINK=		$(LIBTOOL) --mode=link cc -O3 
CCLINK=		$(LIBTOOL) --mode=link cc

LDFLAGS=	
LIBS=		 -lpthread

##################################################
# Targets 
##################################################
all: startup_server startup_client query_stocks update_stocks

##################################################
# Compile and link
##################################################
OBJECTS = benchmark_common.lo benchmark_initial_load.lo populate_portfolios.lo refresh_quotes.lo \
					view_stock_txn.lo view_portfolio_txn.lo purchase_txn.lo sell_txn.lo

benchmark_common.lo: $(SRCDIR)/benchmark_common.c
	$(CC) $(CFLAGS) $?

benchmark_initial_load.lo: $(SRCDIR)/benchmark_initial_load.c
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
# Dump utility
##################################################
benchmark_databases_dump.lo : $(UTILSDIR)/benchmark_databases_dump.c
	$(CC) $(CFLAGS) $?

benchmark_databases_dump : benchmark_databases_dump.lo $(OBJECTS)
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) benchmark_databases_dump.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@

DBDump : benchmark_databases_dump
	$(BINDIR)/benchmark_databases_dump -d STOCKSDB -d PERSONALDB -d CURRENCIESDB -h $(HOMEDIR)/

##################################################
# Query Stocks utility
##################################################
query_stocks.lo : $(UTILSDIR)/query_stocks.c
	$(CC) $(CFLAGS) $?

query_stocks : query_stocks.lo $(OBJECTS)
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) query_stocks.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@

RunQueryStock : query_stocks $(UTILSDIR)/query_stocks_loop.sh
	make init
	rm -rf /tmp/upd*
	sh $(UTILSDIR)/query_stocks_loop.sh	

TestQueryStock : query_stocks $(TESTDIR)/test_query_stocks.sh
	make init
	rm -rf /tmp/upd*
	sh $(TESTDIR)/test_query_stocks.sh

##################################################
# Update Stocks utility
##################################################
update_stocks.lo : $(UTILSDIR)/update_stocks.c
	$(CC) $(CFLAGS) $?

update_stocks : update_stocks.lo $(OBJECTS)
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) update_stocks.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@

RunUpdateStock : update_stocks $(UTILSDIR)/update_stocks_loop.sh
	make init
	rm -rf /tmp/upd*
	sh $(UTILSDIR)/update_stocks_loop.sh	

TestUpdateAll : update_stocks $(TESTDIR)/test_update_all.sh
	make init
	rm -rf /tmp/upd*
	sh $(TESTDIR)/test_update_all.sh

TestUpdateStock : update_stocks $(TESTDIR)/test_update_stocks.sh
	make init
	rm -rf /tmp/upd*
	sh $(TESTDIR)/test_update_stocks.sh

##################################################
# Sell Stocks utility
##################################################
sell_stock.lo : $(UTILSDIR)/sell_stock.c
	$(CC) $(CFLAGS) $?

sell_stock : sell_stock.lo $(OBJECTS)
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) sell_stock.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@


##################################################
# Query portfolios utility
##################################################
query_portfolios.lo : $(UTILSDIR)/query_portfolios.c
	$(CC) $(CFLAGS) $?

query_portfolios : query_portfolios.lo $(OBJECTS)
	$(CCLINK) -o $(BINDIR)/$@ $(LDFLAGS) query_portfolios.lo $(OBJECTS) $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(BINDIR)/$@

Test : 
	make TestQueryStock
	make TestUpdateAll
	make TestUpdateStock

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


.PHONY : clean

clean :
	-rm *.o
	-rm *.lo
	-rm -rf bin
