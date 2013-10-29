#! /usr/bin/python
from os import system
from math import floor
import re
import sys

def tail_fct(extension, seeds, flownum):
   seed=1
   fcts = []
   for seed in range(1, seeds + 1):
     for line in open("../"+str(seed) + "." + extension):
       records = line.split()
       if ((records[0] == "conn") and ((records[1] == flownum) or (records[1] == "*"))) :
         fcts += [float(records[6])]
   fcts.sort()
   return fcts[int(floor(len(fcts) * 0.999))]

def tpt(extension, seeds, flownum):
   tpt=0.0
   count=0
   for seed in range(1, seeds + 1):
     for line in open("../"+str(seed) + "." + extension):
       records = line.split()
       if ((records[0] == "conn:") and ((records[1] == flownum) or (flownum == "*"))) :
         tpt += float(records[11])
         count += 1
   return tpt/count

def delay(extension, seeds, flownum):
   delay=0.0
   count=0
   for seed in range(1, seeds + 1):
     for line in open("../"+str(seed) + "." + extension):
       records = line.split()
       if ((records[0] == "conn:") and ((records[1] == flownum) or (flownum == "*"))) :
         delay += float(records[13])
         count += 1
   return delay/count

if (len(sys.argv) < 2):
  print "Usage: ", sys.argv[0], " [1a|1b|2a|2b|3a|3b] "
  exit(2)
else :
  tablenum=sys.argv[1]

if (tablenum == "1a"):
  # Table 1a
  system("./codelfcfsVscodelFQ-2bulk.sh > /tmp/1a.out 2> /tmp/1a.err")
  print "Table 1a: CoDel+FCFS,       average     throughput: ", tpt("codelfcfs", 1, "*"), " Mbps"
  print "Table 1a: CoDel+FQ,         average     throughput: ", tpt("codel", 1, "*"), " seconds"

elif (tablenum == "1b"):
  # Table 1b
  system("./codelfcfsVscodelFQ-bulk+web.sh > /tmp/1b.out 2> /tmp/1b.err")
  print "Table 1b: CoDel+FCFS,       bulk        throughput: ", tpt("codelfcfs", 74, "0"), " Mbps"
  print "Table 1b: CoDel+FCFS,       tail        FCT:        ", tail_fct("codelfcfs", 74, "1"), " seconds"
  print "Table 1b: CoDel+FQ,         bulk        throughput: ", tpt("codel", 74, "0"), " Mbps"
  print "Table 1b: CoDel+FQ,         tail        FCT:        ", tail_fct("codel", 74, "1"), " seconds"

elif (tablenum == "2a"):
  # Table 2a
  system("./bfqcfq-bulk+web.sh > /tmp/2a.out 2> /tmp/2a.err")
  print "Table 2a: Bufferbloat+FQ,   bulk        throughput: ", tpt("droptail", 74, "0"), " Mbps"
  print "Table 2a: Bufferbloat+FQ,   tail        FCT:        ", tail_fct("droptail", 74, "1"), " seconds"
  print "Table 2a: CoDel+FQ,         average     throughput: ", tpt("codel", 74, "0"), " Mbps" 
  print "Table 2a: CoDel+FQ,         tail        FCT:        ", tail_fct("codel", 74, "1"), " seconds"

elif (tablenum == "2b"):
  # Table 2b
  system("./bfqcfq-int+int.sh > /tmp/2b.out 2> /tmp/2b.err")
  print "Table 2b: Bufferbloat+FQ,   interactive throughput: ", tpt("droptail", 1, "*"), " Mbps"
  print "Table 2b: Bufferbloat+FQ,   interactive RTT:        ", delay("droptail", 1, "*"), " ms"
  print "Table 2b: CoDel+FQ,         interactive throughput: ", tpt("codel", 1, "*"), " Mbps"
  print "Table 2b: CoDel+FQ,         interactive RTT:        ", delay("codel", 1, "*"), " ms"
  
elif (tablenum == "3a"):
  # Table 3a
  system("./bfqcfs.sh > /tmp/3a.out 2> /tmp/3a.err")
  print "Table 3a: Bufferbloat+FQ,   bulk        throughput: ", tpt("droptail", 1, "*"), " Mbps"
  print "Table 3a: CoDel+FQ,         bulk        throughput: ", tpt("codel", 1, "*"), " Mbps"
  
elif (tablenum == "3b"):
  # Table 3b
  system("./bfqcfs.sh > /tmp/3b.out 2> /tmp/3b.err") # Experiment 3a and 3b are the same, so no point rerunning.
  print "Table 3b: Bufferbloat+FQ,   interactive throughput: ", tpt("droptail", 1, "*")," Mbps"
  print "Table 3b: Bufferbloat+FQ,   interactive RTT:        ", delay("droptail", 1, "*"), " ms"
  print "Table 3b: CoDel+FQ,         interactive throughput: ", tpt("codel", 1, "*"), " Mbps"
  print "Table 3b: CoDel+FQ,         interactive RTT:        ", delay("codel", 1, "*"), " ms"
