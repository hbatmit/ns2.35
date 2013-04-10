#! /usr/bin/python

import sys
fh=sys.stdin
first_time=False
for line in fh.readlines() :
  time=int(line.split("=")[1].split(",")[0]);
  if (not first_time) :
    first_ts=time;
    first_time=True;
  print "%.0f"%((time - first_ts)/1.e6);
