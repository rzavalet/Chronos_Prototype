#TODO: Make this more build independent. Eg DEF_LIB, CFLAGS, exampledir and builddir

exampledir= ./src
builddir= ./bin
utilsdir= ./util
homedir= ./databases
datafilesdir= ./datafiles

##################################################
# General library information.
##################################################
DEF_LIB=	-L/usr/local/BerkeleyDB.6.2/lib -ldb-6.2
LIBTOOL=	./libtool

POSTLINK=	$(LIBTOOL) --mode=execute true
#SOLINK=		$(LIBTOOL) --mode=link cc -avoid-version -O3 
SOLINK=		$(LIBTOOL) --mode=link cc -avoid-version -O0

##################################################
# C API.
##################################################
#CFLAGS=		-c -I/usr/local/BerkeleyDB.6.2/include -O3 -g
CFLAGS=		-c -I/usr/local/BerkeleyDB.6.2/include -g
CC=		$(LIBTOOL) --mode=compile cc
#CCLINK=		$(LIBTOOL) --mode=link cc -O3 
CCLINK=		$(LIBTOOL) --mode=link cc

LDFLAGS=	
LIBS=		 -lpthread

##################################################
# Targets 
##################################################
all: startup_server startup_client

##################################################
# Compile and link
##################################################
# TODO: I think these can be improved :P
benchmark_common.lo: $(exampledir)/benchmark_common.c
	$(CC) -I$(exampledir) $(CFLAGS) $?

benchmark_initial_load.lo: $(exampledir)/benchmark_initial_load.c
	$(CC) $(CFLAGS) $?

populate_portfolios.lo:	$(exampledir)/populate_portfolios.c
	$(CC) $(CFLAGS) $?

refresh_quotes.lo:	$(exampledir)/refresh_quotes.c
	$(CC) $(CFLAGS) $?

view_stock_txn.lo:	$(exampledir)/view_stock_txn.c
	$(CC) $(CFLAGS) $?

view_portfolio_txn.lo:	$(exampledir)/view_portfolio_txn.c
	$(CC) $(CFLAGS) $?

purchase_txn.lo:	$(exampledir)/purchase_txn.c
	$(CC) $(CFLAGS) $?

sell_txn.lo:	$(exampledir)/sell_txn.c
	$(CC) $(CFLAGS) $?

startup_server.lo:	$(exampledir)/startup_server.c
	$(CC) $(CFLAGS) $?
startup_server: startup_server.lo benchmark_initial_load.lo populate_portfolios.lo refresh_quotes.lo refresh_quotes.lo view_stock_txn.lo view_portfolio_txn.lo purchase_txn.lo sell_txn.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) startup_server.lo benchmark_initial_load.lo populate_portfolios.lo refresh_quotes.lo view_stock_txn.lo view_portfolio_txn.lo purchase_txn.lo sell_txn.lo benchmark_common.lo $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

startup_client.lo:	$(exampledir)/startup_client.c
	$(CC) $(CFLAGS) $?
startup_client: startup_client.lo benchmark_common.lo 
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) startup_client.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

benchmark_databases_dump.lo: $(exampledir)/benchmark_databases_dump.c
	$(CC) $(CFLAGS) $?
benchmark_databases_dump: benchmark_databases_dump.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) benchmark_databases_dump.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@


##################################################
# Useful targets for running the benchmark
##################################################
init:
	mkdir -p /tmp/chronos/databases
	mkdir -p /tmp/chronos/datafiles
	rm -rf /tmp/chronos/databases/*
	rm -rf /tmp/chronos/datafiles/*
	cp datafiles/* /tmp/chronos/datafiles

dump:
	$(builddir)/benchmark_databases_dump -d STOCKSDB -d PERSONALDB -d CURRENCIESDB -h $(homedir)/

view_stock:
	$(builddir)/view_stock_txn -h $(homedir)/

view_portfolio:
	$(builddir)/view_portfolio_txn -h $(homedir)/

run_refresh_quotes:
	$(utilsdir)/query_quotes.py > $(datafilesdir)/quotes.txt
	$(builddir)/refresh_quotes -b $(datafilesdir)/ -h $(homedir)/

place_order:
	$(builddir)/purchase_txn -h $(homedir)/

sell_stocks:
	$(builddir)/sell_txn -h $(homedir)/

clean:
	-rm *.o
	-rm *.lo
	-rm -rf bin
