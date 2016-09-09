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

1) benchmark_initial_load: Performs the initial load of the 'static' tables.

2) benchmark_databases_dump: Dumps the contents of the tables.

3) view_stock_txn: Implementation of the View Stock transaction described in the paper

4) view_portfolio_txn: Implementation of the View Portfolio transaction described in the paper

5) refresh_quotes: Implementation of the Update transaction described in the paper

6) populate_portfolios: Performs an initial population of the portfolios table using dummy data

[In Progress]

How to run the prototype
========================
1) Initially create the system tables:

	$ make reinit
	$ make load
	$ make run_refresh_quotes
	$ make run_populate_portfolios

2) Execute one of the transactions, e.g.
	$ make view_stock

Compiling
==========
See the INSTALL document

History
========
Chronos was proposed by Khan et al. in 2007. 

This repository is an independent attempt of recreating the experiments presented in the 
original paper. Please report any problem to <zavaleta.vazquez.ricardo.jose@gmail.com>
