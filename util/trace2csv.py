#!/scratch/rzavalet/usr/bin/python
"""
 Usage: python trace2csv.py tracefile
 -----
 input: trace format
 output: csv format
"""

import sys
import re

def extract_data2(s):
    pattern = re.compile(r"^\[THR: (?P<tid>.*)\] \[TYPE: (?P<type>.*)\] \[ENQUEUE: (?P<ts0>.*)\] \[START: (?P<ts1>.*)\] \[END: (?P<ts2>.*)\] \[EXECUTION: (?P<ts3>.*)\] \[DURATION: (?P<ts4>.*)\]$", re.VERBOSE)

    match = pattern.match(s)
    if not match:
      print >> sys.stderr, "Could not filter line: %s\n" % (s)
    try:
      tid = match.group("tid")
      txn_type = match.group("type")
      ts0 = match.group("ts0")
      ts1 = match.group("ts1")
      ts2 = match.group("ts2")
      ts3 = match.group("ts3")
      ts4 = match.group("ts4")

      return (tid, txn_type, ts0, ts1, ts2, ts3, ts4)
    except Exception, e:
      print >> sys.stderr, "Could not filter line: %s\n%s" % (s, e)


if __name__ == '__main__':
  argvs = sys.argv
  argcs = len(argvs)
  if argcs != 2:
    sys.stderr.write('Usage: python %s trace \n' % argvs[0])
    sys.exit(1)

  filename = argvs[1]

  infile = open(filename, 'r')
  inlines = infile.readlines()
  infile.close()

  do_header = 1
  outlines = []
  for line in inlines:

    (tid, txn_type, ts0, ts1, ts2, ts3, ts4) = extract_data2(line)
    line = '' + tid + ',' + txn_type + ',' + ts0 + ',' + ts1 + ',' + ts2 + ',' + ts3 + ',' + ts4
    outlines.append(line)

  outfile = open(filename + '_csv', 'w')
  outfile.writelines(outlines)
  outfile.close()

