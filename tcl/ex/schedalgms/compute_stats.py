#! /usr/bin/python
import sys,re,numpy
from math import log
fh=sys.stdin
reqd_tag=sys.argv[1]
reqd_stat=sys.argv[2]
prop_delay_ms=float(sys.argv[3])
stats=[]
for line in fh.readlines():
  line.strip();
  tag = line.split()[0]
  
  if (line.find("sndrtt") == -1):                      # Process only the stats lines
    continue
  elif ( (tag != reqd_tag) and (reqd_tag != "all") ) : # Process iff you have the right tag
    continue
  else:
    if (reqd_stat == "fct"):
      stats += [float((re.search('(?<=fctMs: )\d+', line)).group(0))]
    elif (reqd_stat == "tpt"):
      stats += [float((re.search('(?<=aMbps: )\d+\.\d+', line)).group(0))]
    elif (reqd_stat == "tail"):
      stats += [float((re.search('(?<=rtt95th )\d+\.\d+', line)).group(0))];
    elif (reqd_stat == "rtt"):
      stats += [float((re.search('(?<=sndrttMs )\d+\.\d+', line)).group(0))]
    elif (reqd_stat == "utility"):
      throughput  = float((re.search('(?<=aMbps: )\d+\.\d+', line)).group(0))
      tail_delay  = float((re.search('(?<=rtt95th )\d+\.\d+', line)).group(0))
      mean_delay  = float((re.search('(?<=sndrttMs )\d+\.\d+', line)).group(0))
      fct         = float((re.search('(?<=fctMs: )\d+', line)).group(0))
      if (tag == "web"):
        util = log(1000000) - log(fct)
      elif (tag == "bt"):
        util = log(1000000) + log(throughput)
      elif (tag == "stream"):
        util = log(1000000) + log(throughput) - log(tail_delay - prop_delay_ms)
      stats += [util]
#      stats += [float((re.search('(?<=s_util: )\d+\.\d+', line)).group(0))]
    else :
      print "Invalid stat requested"
      exit(5)
print reqd_stat,"Median:%.2f Mean:%.2f sd:%.2f samples %d"% (numpy.median(stats), numpy.mean(stats), numpy.std(stats), len(stats))
