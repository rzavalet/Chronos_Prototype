#TODO: Make this more build independent. Eg DEF_LIB, CFLAGS, exampledir and builddir

exampledir= ./src
builddir= ./bin

##################################################
# General library information.
##################################################
DEF_LIB=	-L/usr/local/BerkeleyDB.6.1/lib -ldb-6.1
LIBTOOL=	./libtool

POSTLINK=	$(LIBTOOL) --mode=execute true
SOLINK=		$(LIBTOOL) --mode=link cc -avoid-version -O3 

##################################################
# C API.
##################################################
CFLAGS=		-c -I/usr/local/BerkeleyDB.6.1/include -O3 -g
CC=		$(LIBTOOL) --mode=compile cc
CCLINK=		$(LIBTOOL) --mode=link cc -O3 

LDFLAGS=	
LIBS=		 -lpthread

##################################################
# Targets for example programs.
##################################################
examples_c: benchmark_initial_load benchmark_databases_dump

##################################################
# Example programs for C.
##################################################
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

gettingstarted_common.lo: $(exampledir)/gettingstarted_common.c
	$(CC) -I$(exampledir) $(CFLAGS) $?
example_database_load.lo: $(exampledir)/example_database_load.c
	$(CC) $(CFLAGS) $?
example_database_read.lo: $(exampledir)/example_database_read.c
	$(CC) $(CFLAGS) $?
example_database_load: example_database_load.lo gettingstarted_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) example_database_load.lo gettingstarted_common.lo  $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@
example_database_read: example_database_read.lo gettingstarted_common.lo
	$(CCLINK) -o $(builddir)/$@ $(LDFLAGS) example_database_read.lo gettingstarted_common.lo $(DEF_LIB) $(LIBS)
	$(POSTLINK) $(builddir)/$@

clean:
	-rm example_database_load example_database_read
	-rm *.o
	-rm *.lo
	-rm *.db
	-rm -rf bin
	-rm -rf databases/*
