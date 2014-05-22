#! /usr/bin/python
import os
import numpy
import math

# constants
iteration_count = 50
resultfolder = "resultsoptimal"
topofolder = "topooptimal"

# protocols
rationalstr="-tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail"
cubicstr="-tcp TCP/Linux/cubic -sink TCPSink/Sack1 -gw DropTail"
cubicsfqCoDelstr="-tcp TCP/Linux/cubic -sink TCPSink/Sack1 -gw sfqCoDel"

# traffic pattern
traffic_workload="-ontype time -onrand Exponential -onavg 1.0 -offrand Exponential"

# Clean up the results and topologies folder
assert(os.path.isdir(resultfolder) == False)
os.system("mkdir " + resultfolder)
assert(os.path.isdir(topofolder) == False)
os.system("mkdir " + topofolder)

# Make delay toplogies
fh=open(topofolder + "/delay75.txt", "w");
fh.write("0 1 " + str(10 ** 1.5) + " " + 75 + "\n");
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

# Run all protocols on single configuration
topology = topofolder + "/delay75.txt"
for run in range(1, iteration_count + 1):
  synthesize( "/home/anirudh/camera-ready/rats/rtt-150-alone.dna.5",     topology,     rationalstr,      traffic_workload, 1.0, 100, run, "remy");
  synthesize( "NULL",                                                    topology,     cubicstr,         traffic_workload, 1.0, 100, run, "cubic");
  synthesize( "NULL",                                                    topology,     cubicsfqCoDelstr, traffic_workload, 1.0, 100, run, "cubicsfqCoDel");

print "all: " + synthesize.targets, synthesize.cmdlines
