# Chronos_Prototype
This is an attempt to recreate the prototype created by Kang et al, 2007

In 2007, Kang et al. presented an initial version of a RTDB Testbed. They called
it Chronos. This Testbed was written to work on top of BerkeleyDB, which is an
open source database. The paper describes both the schema of the tables used, as
well as the main transaction types involved. The whole testbed models online
stock trades.

In this repository, I am trying to reconstruct their testbed from scratch. I am
providing the following utilities:

1) benchmark_initial_load: Performs the initial load of the 'static' tables.

2) benchmark_databases_dump: Dumps the contents of the tables.

[In Progress]
