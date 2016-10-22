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
SOLINK=		$(LIBTOOL) --mode=link cc -avoid-version -O3 

##################################################
# C API.
##################################################
CFLAGS=		-c -I/usr/local/BerkeleyDB.6.2/include -O3 -g
CC=		$(LIBTOOL) --mode=compile cc
CCLINK=		$(LIBTOOL) --mode=link cc -O3 

LDFLAGS=	
LIBS=		 -lpthread

##################################################
# Targets 
##################################################
all: benchmark_initial_load benchmark_databases_dump view_stock_txn view_portfolio_txn refresh_quotes populate_portfolios purchase_txn sell_txn

##################################################
# Compile and link
##################################################
# TODO: I think these can be improved :P
benchmark_common.lo: $(exampledir)/benchmark_common.c
	$(CC) -I$(exampledir) $(CFLAGS) $?

benchmark_initial_load.lo: $(exampledir)/benchmark_initial_load.c
	$(CC) $(CFLAGS) $?
benchmark_initial_load: benchmark_initial_load.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) benchmark_initial_load.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

benchmark_databases_dump.lo: $(exampledir)/benchmark_databases_dump.c
	$(CC) $(CFLAGS) $?
benchmark_databases_dump: benchmark_databases_dump.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) benchmark_databases_dump.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

view_stock_txn.lo:	$(exampledir)/view_stock_txn.c
	$(CC) $(CFLAGS) $?
view_stock_txn: view_stock_txn.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) view_stock_txn.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

view_portfolio_txn.lo:	$(exampledir)/view_portfolio_txn.c
	$(CC) $(CFLAGS) $?
view_portfolio_txn: view_portfolio_txn.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) view_portfolio_txn.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

refresh_quotes.lo:	$(exampledir)/refresh_quotes.c
	$(CC) $(CFLAGS) $?
refresh_quotes: refresh_quotes.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) refresh_quotes.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

populate_portfolios.lo:	$(exampledir)/populate_portfolios.c
	$(CC) $(CFLAGS) $?
populate_portfolios: populate_portfolios.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) populate_portfolios.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

purchase_txn.lo:	$(exampledir)/purchase_txn.c
	$(CC) $(CFLAGS) $?
purchase_txn: purchase_txn.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) purchase_txn.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

sell_txn.lo:	$(exampledir)/sell_txn.c
	$(CC) $(CFLAGS) $?
sell_txn: sell_txn.lo benchmark_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) sell_txn.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

startup_client.lo:	$(exampledir)/startup_client.c
	$(CC) $(CFLAGS) $?
startup_client: startup_client.lo benchmark_common.lo 
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) startup_client.lo benchmark_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@


##################################################
# Useful targets for running the benchmark
##################################################
reinit:
	rm -rf $(homedir)/*
	make load
	make run_refresh_quotes
	make run_populate_portfolios

load:
	-mkdir databases
	$(builddir)/benchmark_initial_load -b $(datafilesdir)/ -h $(homedir)/

dump:
	$(builddir)/benchmark_databases_dump -d STOCKSDB -d PERSONALDB -d CURRENCIESDB -h $(homedir)/

view_stock:
	$(builddir)/view_stock_txn -h $(homedir)/

view_portfolio:
	$(builddir)/view_portfolio_txn -h $(homedir)/

run_refresh_quotes:
	$(utilsdir)/query_quotes.py > $(datafilesdir)/quotes.txt
	$(builddir)/refresh_quotes -b $(datafilesdir)/ -h $(homedir)/

run_populate_portfolios:
	$(builddir)/populate_portfolios -h $(homedir)/

place_order:
	$(builddir)/purchase_txn -h $(homedir)/

sell_stocks:
	$(builddir)/sell_txn -h $(homedir)/

clean:
	-rm *.o
	-rm *.lo
	-rm -rf bin
