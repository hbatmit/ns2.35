#! /usr/bin/python
import os
import numpy
import math

# constants
iteration_count = 10
resultfolder = "resultsmultilink"
topofolder = "topomultilink"

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

# define a logrange function
def logrange(below, above, num_points):
  start = math.log10(below)
  stop  = math.log10(above)
  return list(numpy.around(numpy.logspace(start, stop, num_points), 5));

# Make linkspeed topologies
linkspeed_range = logrange(10.0, 100.0, 32);

for link1 in linkspeed_range:
  for link2 in linkspeed_range:
    fh=open(topofolder + "/linkspeed-"+ str(link1) + "-" + str(link2) + ".txt", "w");
    # The main links
    fh.write("0 1 " + str(link1) + " 75\n");
    fh.write("1 2 " + str(link2) + " 75\n");

    fh.close();

# Make common sd file
fh=open(topofolder + "/sd.txt", 'w');
fh.write("0 2\n"); # traverses link1 and link2
fh.write("0 1\n"); # traverses link1 alone
fh.write("1 2\n"); # traverses link2 alone
fh.close();

# Synthesize command line
def synthesize( whiskertree, topology, tcp_agents, traffic_cfg, off_time, sim_time, run, tag ):
  global topofolder
  global resultfolder
  cmdline="WHISKERS=" + whiskertree + " pls ./decompose.tcl " + topology + " " + topofolder + "/sd.txt " + tcp_agents + " " +  traffic_cfg + " -offavg "+ str( off_time ) + " -simtime " + str( sim_time ) + " -run " + str( run )
  fileio=" >" + resultfolder + "/" + tag + "run" + str( run ) + ".out " + "2>" + resultfolder + "/" + tag + "run" + str( run ) +  ".err"
  cmdline += fileio
  target = resultfolder + "/" + tag + "run" + str( run ) + ".out"
  synthesize.targets += " " + target
  synthesize.cmdlines += "\n" + target + ":\n\t" + cmdline
synthesize.targets=""
synthesize.cmdlines=""

# Auto-agility
for link1 in linkspeed_range:
  for link2 in linkspeed_range:
    linkspeed_topology = topofolder + "/linkspeed-" + str(link1) + "-" + str(link2) + ".txt"
    for run in range(1, iteration_count + 1):
      synthesize( "/home/am2/anirudh/multilink-take2.dna.2", linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "multilinkremy" + str(link1) + "-" + str(link2));
      synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",   linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "10x-link" + str(link1) + "-" + str(link2));
      synthesize( "NULL",                                    linkspeed_topology, cubicsfqCoDelstr, traffic_workload, 1.0, 100, run, "cubicsfqCoDel-link" + str(link1) + "-" + str(link2));
      synthesize( "NULL",                                    linkspeed_topology, cubicstr,         traffic_workload, 1.0, 100, run, "cubic-link" + str(link1) + "-" + str(link2));

print "all: " + synthesize.targets, synthesize.cmdlines
