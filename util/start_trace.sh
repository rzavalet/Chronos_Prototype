#!/bin/bash

liblitmus=/home/ricardo/working_dir/liblitmus
fttrace=/home/ricardo/working_dir/ft_tools
rootdir=..
traceout=mytrace

echo "Setting scheduler..."
${liblitmus}/setsched GSN-EDF
${liblitmus}/showsched

rm mytrace*
rm *.pdf
${fttrace}/st-trace-schedule ${traceout}
${fttrace}/st-draw *.bin
