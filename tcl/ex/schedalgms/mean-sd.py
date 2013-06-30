#! /usr/bin/python
# Compute mean and sd of per user average rtt's and per user average throughputs
import sys,re,numpy
fh=sys.stdin
utils=[]
fcts =[]
tail_rtts=[]
for line in fh.readlines():
  line.strip();
  tag = line.split()[0]
  if (tag == "web") :
    fct = int((re.search('(?<=fctMs: )\d+', line)).group(0));
    fcts  += [fct]
  if (tag == "bt") :
    util    = float((re.search('(?<=utilization: )\d+\.\d+', line)).group(0));
    utils += [util]

print "Fct Median:%.2f Mean:%.2f sd:%.2f"% (numpy.median(fcts), numpy.mean(fcts),  numpy.std(fcts))
print "Utils Median:%.2f Mean:%.2f sd:%.2f"% (numpy.median(utils), numpy.mean(utils), numpy.std(utils))
