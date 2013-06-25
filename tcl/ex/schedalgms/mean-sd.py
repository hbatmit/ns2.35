#! /usr/bin/python
# Compute mean and sd of per user average rtt's and per user average throughputs
import sys,re,numpy
fh=sys.stdin
utils=[]
rtts=[]
tail_rtts=[]
for line in fh.readlines():
  line.strip();
  util    = float((re.search('(?<=utilization: )\d+\.\d+', line)).group(0));
  snd_rtt = float((re.search('(?<=sndrttMs )\d+\.\d+', line)).group(0));
  rtt_95th = float((re.search('(?<=rtt95th )\d+\.\d+', line)).group(0));
  utils += [util]
  rtts  += [snd_rtt]
  tail_rtts += [rtt_95th]

print "Delay Median:%.2f Mean:%.2f sd:%.2f"% (numpy.median(rtts), numpy.mean(rtts),  numpy.std(rtts))
print "95th percentile Delay Median:%.2f Mean:%.2f sd:%.2f"% (numpy.median(tail_rtts), numpy.mean(tail_rtts),  numpy.std(tail_rtts))
print "Utils Median:%.2f Mean:%.2f sd:%.2f"% (numpy.median(utils), numpy.mean(utils), numpy.std(utils))
