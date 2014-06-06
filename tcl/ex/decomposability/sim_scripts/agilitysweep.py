#! /usr/bin/python
import os
import numpy
import math

# constants
iteration_count = 10
resultfolder = "results-rttvar"
topofolder = "topo-rttvar"

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
for delay in range(1, 151, 1):
  fh=open(topofolder + "/delay"+ str(delay) + ".txt", "w");
  fh.write("0 1 " + str(10 ** 1.5) + " " + str(delay) + "\n");
  fh.close();

# Make common sd file
fh=open(topofolder + "/sd.txt", 'w');
fh.write("0 1\n");
fh.write("0 1\n");
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
for delay in range(1, 151, 1):
  delay_topology = topofolder + "/delay" + str(delay) + ".txt"
  for run in range(1, iteration_count + 1):
    synthesize( "/data/lsp/owenhsin/anirudh/camera-ready/150-alone/150-alone.dna.2", delay_topology, rationalstr, traffic_workload, 1.0, 100, run, "rtt150"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/camera-ready/140160/145--155.dna.4", delay_topology, rationalstr, traffic_workload, 1.0, 100, run, "rtt145--155"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/camera-ready/140160/140-160.dna.5",     delay_topology, rationalstr, traffic_workload, 1.0, 100, run, "rtt140--160"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-config-1/rtt_10x.dna.3",    delay_topology, rationalstr, traffic_workload, 1.0, 100, run, "rtt110--200"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-config-2/rtt_20x.dna.4",    delay_topology, rationalstr, traffic_workload, 1.0, 100, run, "rtt50--250"+str(delay));
    synthesize( "/data/lsp/owenhsin/anirudh/prthaker/rtt-config-3/rtt_30x.dna.2",    delay_topology, rationalstr, traffic_workload, 1.0, 100, run, "rtt10--280"+str(delay));
    synthesize( "NULL",    delay_topology,     cubicstr,              traffic_workload, 1.0, 100, run, "cubic" + str(delay));
    synthesize( "NULL",    delay_topology,     cubicsfqCoDelstr,      traffic_workload, 1.0, 100, run, "cubicsfqCoDel" + str(delay));

print "all: " + synthesize.targets, synthesize.cmdlines
