#!/bin/perl

use strict;
use warnings;

open(FILE1, $ARGV[0]);

print "ACC_DURATION_MS, NUM_TXN, AVG_DURATION_MS, NUM_FAILED_TXNS, NUM_TIMELY_TXNS, DELTA, DELTA_S, TNX_ENQUEUED, TXN_TO_WAIT\n";
while (<FILE1>) {
  my $line=$_;
  
  my ($accdur, $num, $avgdur, $numfail, $numtimely, $delta, $deltas, $numenq, $numwait) = ($line =~ /\[ACC_DURATION_MS: (\d+\.\d+)\], \[NUM_TXN: (\d+)\], \[AVG_DURATION_MS: (\d+\.\d+)\] \[NUM_FAILED_TXNS: (\d+)\], \[NUM_TIMELY_TXNS: (\d+)\] \[DELTA\(k\): (\d+\.\d+)\], \[DELTA_S\(k\): (\d+\.\d+)\] \[TNX_ENQUEUED: (\d+)\] \[TXN_TO_WAIT: (\d+)\]/);
	print "$accdur, $num, $avgdur, $numfail, $numtimely, $delta, $deltas, $numenq, $numwait\n";
}

close FILE1;

