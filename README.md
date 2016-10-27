# Chronos_Prototype

What is Chronos_Prototype
==========================
This is an attempt to recreate the system proposed by Kang et al, 2007 (See http://www.cs.binghamton.edu/~kang/rtcsa07.pdf)

In 2007, Kang et al. presented an initial version of a RTDB Testbed. They called
it Chronos. This Testbed was written to work on top of BerkeleyDB, which is an
open source database. The paper describes both the schema of the tables used, as
well as the main transaction types involved. The whole testbed models online
stock trades.

What is included in the prototype
=================================
In this repository, I am trying to reconstruct their testbed from scratch. I am
providing the following utilities:

1) benchmark_databases_dump: Dumps the contents of the tables.

2) startup_server: Starts up a chronos server process which is able to accept incoming transaction request from multiple chronos clients.

3) startup_client: Starts up a chronos client process which connects to a chronos server and sends transaction requests.


How to run the prototype
========================
1) Initially create the required directories:

	$ make init

2) Startup a server process in one terminal:
	$ ./startup_server OPTIONS

3) In other terminals, spawn client processes:
  	$ ./startup_client OPTIONS

Compiling
==========
See the INSTALL document

History
========
Chronos was proposed by Khan et al. in 2007. 

This repository is an independent attempt of recreating the experiments presented in the 
original paper. Please report any problem to <zavaleta.vazquez.ricardo.jose@gmail.com>
