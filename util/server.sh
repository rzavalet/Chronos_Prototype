#!/bin/bash

FILEOUT=myfile.txt
FILESAMPLE=mysample.txt

../bin/startup_server -c $1 -u 50 -s 5 -p $2 -n -m 0 2>&1 | tee $FILEOUT
grep -P 'SAMPLING' $FILEOUT > $FILESAMPLE ; perl sample2csv.pl $FILESAMPLE | head -n 13
