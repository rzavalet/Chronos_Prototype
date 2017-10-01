#!/bin/perl

use strict;
use warnings;

open(FILE1, $ARGV[0]);

my $deadline = 500; # in ms

my $cnt_missed = 0;
my $avg_duration = 0;
my $cnt = 0;

while (<FILE1>) {
  my $line=$_;
  my ($tid, $txn_type, $ts0, $ts1, $ts2, $ts3, $ts4) = ($line =~ /^\[THR: (\d+)\] \[TYPE: (\w+)\] \[ENQUEUE: (\d+)\] \[START: (\d+)\] \[END: (\d+)\] \[EXECUTION: (\d+)\] \[DURATION: (\d+)\]$/);
	print "$tid, $txn_type, $ts0, $ts1, $ts2, $ts3, $ts4\n";

  $cnt ++;
  $avg_duration += $ts4;

  if ($ts4 > $deadline) {
    $cnt_missed += 1;
  }
}

close FILE1;

$avg_duration /= $cnt;
print "AVG_DURATION: $avg_duration\n";
print "NUM MISSED: $cnt_missed\n";
