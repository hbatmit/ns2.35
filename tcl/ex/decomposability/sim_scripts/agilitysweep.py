#! /usr/bin/python
import os
import numpy
import math

# constants
iteration_count = 10
resultfolder = "resultslogarithmic"
topofolder = "topologarithmic"

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

# Make delay toplogies
for delay in range(5, 151, 5):
  fh=open(topofolder + "/delay"+ str(delay) + ".txt", "w");
  fh.write("0 1 " + str(10 ** 1.5) + " " + str(delay) + "\n");
  fh.write("1 2 1000000000 1\n");
  fh.write("1 3 1000000000 1\n");
  fh.close();

# Make common sd file
fh=open(topofolder + "/sd.txt", 'w');
fh.write("0 2\n");
fh.write("0 3\n");
fh.close();

# Synthesize command line
def synthesize( whiskertree, topology, tcp_agents, traffic_cfg, off_time, sim_time, run, tag ):
  global topofolder
  global resultfolder
  cmdline="WHISKERS=" + whiskertree + " ./decompose.tcl " + topology + " " + topofolder + "/sd.txt " + tcp_agents + " " +  traffic_cfg + " -offavg "+ str( off_time ) + " -simtime " + str( sim_time ) + " -run " + str( run )
  fileio=" >" + resultfolder + "/" + tag + "run" + str( run ) + ".out " + "2>" + resultfolder + "/" + tag + "run" + str( run ) +  ".err"
  cmdline += fileio
  target = resultfolder + "/" + tag + "run" + str( run ) + ".out"
  synthesize.targets += " " + target
  synthesize.cmdlines += "\n" + target + ":\n\t" + cmdline
synthesize.targets=""
synthesize.cmdlines=""

# Cross-agility on delay
for delay in range(5, 151, 5):
  delay_topology = topofolder + "/delay" + str(delay) + ".txt"
  for run in range(1, iteration_count + 1):
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-config-1/rtt_10x.dna.3",     delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "rtt10x"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-config-2/rtt_20x.dna.4",     delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "rtt20x"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-config-3/rtt_30x.dna.2",    delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "rtt30x"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-and-link-speed/rtt_link.dna.3",    delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "rttandlinkspeed"+str(delay));

print "all: " + synthesize.targets, synthesize.cmdlines
