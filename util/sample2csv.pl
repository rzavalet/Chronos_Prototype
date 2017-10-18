#!/bin/perl

use strict;
use warnings;

my $avg_duration = 0;
my $avg_success_rate = 0;
my $avg_timely_txn_rate = 0;
my $avg_txn = 0;
my $txn_cnt = 0;
my $samplesize = 0;

open(FILE1, $ARGV[0]);

print "ACC_DURATION_MS, NUM_TXN, AVG_DURATION_MS, NUM_FAILED_TXNS, NUM_TIMELY_TXNS, DELTA, DELTA_S, TNX_ENQUEUED, TXN_TO_WAIT\n";
while (<FILE1>) {
  my $line=$_;
  
  my ($accdur, $num, $avgdur, $numfail, $numtimely, $delta, $deltas, $numenq, $numwait) = ($line =~ /\[ACC_DURATION_MS: (\d+\.\d+)\], \[NUM_TXN: (\d+)\], \[AVG_DURATION_MS: (\d+\.\d+)\] \[NUM_FAILED_TXNS: (\d+)\], \[NUM_TIMELY_TXNS: (\d+)\] \[DELTA\(k\): (\d+\.\d+)\], \[DELTA_S\(k\): (\d+\.\d+)\] \[TNX_ENQUEUED: (\d+)\] \[TXN_TO_WAIT: (\d+)\]/);
	print "$accdur, $num, $avgdur, $numfail, $numtimely, $delta, $deltas, $numenq, $numwait\n";

  $avg_duration += $avgdur;

  if ($num == 0) {
    $avg_success_rate += 100 *($num - $numfail)/$num;
  }
  else {
    $avg_success_rate += 0;
  }
  if ($num == 0) {
    $avg_timely_txn_rate += 100 * $numtimely/$num;
  }
  else {
    $avg_timely_txn_rate += 0;
  }
  $txn_cnt += $num;
  $samplesize ++;
}

$avg_duration /= $samplesize;
$avg_success_rate /= $samplesize;
$avg_timely_txn_rate /= $samplesize;
$txn_cnt /= $samplesize;

print "\n";
print "### $avg_duration, $avg_success_rate, $avg_timely_txn_rate, $txn_cnt\n";
close FILE1;

