#! /usr/bin/python
from math import log

remy1000x=open("plots/random1000x.plot", "r") ;
cubic=open("plots/randomcubic.plot", "r") ;
cubicsfqCoDel=open("plots/randomcubicsfqCoDel.plot", "r") ;

remytpts = dict()
cubictpts = dict()
cubicsfqCoDeltpts = dict()

remydels = dict()
cubicdels = dict()
cubicsfqCoDeldels = dict()

for line in remy1000x.readlines():
  records = line.split()
  remytpts[int(records[0])] = float(records[1]);
  remydels[int(records[0])] = float(records[2]);

for line in cubic.readlines():
  records = line.split()
  cubictpts[int(records[0])] = float(records[1]);
  cubicdels[int(records[0])] = float(records[2]);

for line in cubicsfqCoDel.readlines():
  records = line.split()
  cubicsfqCoDeltpts[int(records[0])] = float(records[1]);
  cubicsfqCoDeldels[int(records[0])] = float(records[2]);

cubic_fh  = open("cubic.improvements", "w");
cubicsfqCoDel_fh = open("cubicsfqCoDel.improvements", "w"); 
for entry in remytpts:
  print>>cubic_fh, remytpts[entry]/cubictpts[entry], cubicdels[entry]/remydels[entry]
  print>>cubicsfqCoDel_fh, remytpts[entry]/cubicsfqCoDeltpts[entry], cubicsfqCoDeldels[entry]/remydels[entry]
