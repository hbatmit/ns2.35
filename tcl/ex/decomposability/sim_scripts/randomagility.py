#! /usr/bin/python
import os
import numpy
import math
import random

# constants
iteration_count = 10
resultfolder = "resultsrandom"
topofolder = "toporandom"

# protocols
rationalstr="-tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail"
cubicstr="-tcp TCP/Linux/cubic -sink TCPSink/Sack1 -gw DropTail"
cubicsfqCoDelstr="-tcp TCP/Linux/cubic -sink TCPSink/Sack1 -gw sfqCoDel"

# traffic pattern
traffic_workload="-ontype time -onrand Exponential -onavg 1.0 -offrand Exponential"

# Clean up the results and topologies folder
os.system("rm -rf " + resultfolder)
os.system("mkdir " + resultfolder)
os.system("rm -rf " + topofolder)
os.system("mkdir " + topofolder)

# Make it deterministically random, can't sleep properly at night otherwise
random.seed(1)
off_avg=[0]*1000
for i in range(0, 1000):
  # Sample link speed from 1 to 1000, log-uniform
  log_link_speed = random.uniform(math.log(1), math.log(1000))
  link_speed = math.exp(log_link_speed); 

  # Sample workload from 0 to 1
  onpercent = random.uniform(0.0, 1.0)
  off_avg[i] = ( 100.0 - onpercent ) / onpercent;
 
  # Sample multiplexing from 1 to 20
  muxing = random.randint(1, 20)
 
  # Sample RTT from 10 through 300 ms
  rtt = random.uniform(10, 300)

  fh = open(topofolder + "/toporand" + str(i) + ".txt", "w");
  fh.write("0 1 " + str(link_speed) + " " + str(rtt/2.0) + "\n");
  fh.close()

  fh = open(topofolder + "/sdrand" + str(i) + ".txt", "w");
  for i in range(1, muxing + 1):
    fh.write("0 1\n");
  fh.close();

# Synthesize command line
def synthesize( whiskertree, topology, sdpairs, tcp_agents, traffic_cfg, off_time, sim_time, run, tag ):
  global resultfolder
  cmdline="WHISKERS=" + whiskertree + " pls ./decompose.tcl " + topology + " " + sdpairs + " " + tcp_agents + " " +  traffic_cfg + " -offavg "+ str( off_time ) + " -simtime " + str( sim_time ) + " -run " + str( run )
  fileio=" >" + resultfolder + "/" + tag + "run" + str( run ) + ".out " + "2>" + resultfolder + "/" + tag + "run" + str( run ) +  ".err"
  cmdline += fileio
  target = resultfolder + "/" + tag + "run" + str( run ) + ".out"
  synthesize.targets += " " + target
  synthesize.cmdlines += "\n" + target + ":\n\t" + cmdline
synthesize.targets=""
synthesize.cmdlines=""

# Auto-agility
for i in range(0, 1000):
  topology = topofolder + "/toporand" + str(i) + ".txt"
  sdpairs  = topofolder + "/sdrand" + str(i) + ".txt"
  for run in range(1, iteration_count + 1):
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     topology, sdpairs, rationalstr,      traffic_workload, off_avg[i], 100, run, "1000x-rand"+str(i));
    synthesize( "NULL",                                   topology, sdpairs, cubicsfqCoDelstr, traffic_workload, off_avg[i], 100, run, "cubicsfqCoDel-rand"+str(i));
    synthesize( "NULL",                                   topology, sdpairs, cubicstr,         traffic_workload, off_avg[i], 100, run, "cubic-rand"+str(i));

print "all: " + synthesize.targets, synthesize.cmdlines
