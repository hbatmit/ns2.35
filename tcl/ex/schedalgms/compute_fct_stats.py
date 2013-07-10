# Stats for flow completion time
#! /usr/bin/python
import sys,re,numpy
from math import floor
fh=sys.stdin
stats=numpy.array([])
reqd_tag=sys.argv[1]
for line in fh.readlines():
  line.strip()
  if (len(line.split()) <= 1):               # If you have lesser than 2 records, continue
    continue
  elif (line.find("updating pkts") == -1):   # Process only lines with "updating pkts"
    continue
  elif ( (line.split()[1] != reqd_tag) and (reqd_tag != "all") ) :
                                             # ignore any lines without the right tag
    continue
  else:
    print line 
    stats = numpy.append(stats, [float((re.search('(?<=time )\d+\.?\d*', line)).group(0))]);
    print stats

print "FCT Tail: %.2f Median:%.2f Mean:%.2f sd:%.2f samples %d"% (numpy.sort(stats)[floor(0.999*len(stats))], numpy.median(stats), numpy.mean(stats), numpy.std(stats), len(stats))
