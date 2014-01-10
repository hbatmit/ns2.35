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

# define a logrange function
def logrange(below, above, num_points):
  start = math.log10(below)
  stop  = math.log10(above)
  return list(numpy.around(numpy.logspace(start, stop, num_points), 5));

# Make linkspeed topologies
linkspeed_range = logrange(1.0, 1000, 1000);
for linkspeed in linkspeed_range:
  fh=open(topofolder + "/linkspeed"+ str(linkspeed) + ".txt", "w");
  fh.write("0 1 " + str(linkspeed) + " 74\n");
  fh.write("1 2 1000000000 1\n");
  fh.write("1 3 1000000000 1\n");
  fh.close();

# Make delay toplogies
for delay in range(5, 151, 5):
  fh=open(topofolder + "/delay"+ str(delay) + ".txt", "w");
  fh.write("0 1 " + str(10 ** 1.5) + " " + str(delay) + "\n");
  fh.write("1 2 1000000000 1\n");
  fh.write("1 3 1000000000 1\n");
  fh.close();

# Make dutycylce topology
fh=open(topofolder + "/dutycycle.txt", "w");
fh.write("0 1 " + str(10 ** 1.5) + " 74\n");
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
  cmdline="WHISKERS=" + whiskertree + " pls ./decompose.tcl " + topology + " " + topofolder + "/sd.txt " + tcp_agents + " " +  traffic_cfg + " -offavg "+ str( off_time ) + " -simtime " + str( sim_time ) + " -run " + str( run )
  fileio=" >" + resultfolder + "/" + tag + "run" + str( run ) + ".out " + "2>" + resultfolder + "/" + tag + "run" + str( run ) +  ".err"
  cmdline += fileio
  target = resultfolder + "/" + tag + "run" + str( run ) + ".out"
  synthesize.targets += " " + target
  synthesize.cmdlines += "\n" + target + ":\n\t" + cmdline
synthesize.targets=""
synthesize.cmdlines=""

# Auto-agility
for linkspeed in linkspeed_range:
  linkspeed_topology = topofolder + "/linkspeed" + str(linkspeed) + ".txt"
  for run in range(1, iteration_count + 1):
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "1000x-link"+str(linkspeed));
    synthesize( "/home/am2/anirudh/bigbertha-100x.dna.5", linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "100x-link"+str(linkspeed));
    synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",  linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "10x-link"+str(linkspeed));
    synthesize( "NULL",                                   linkspeed_topology, cubicsfqCoDelstr, traffic_workload, 1.0, 100, run, "cubicsfqCoDel-link"+str(linkspeed));
    synthesize( "NULL",                                   linkspeed_topology, cubicstr,         traffic_workload, 1.0, 100, run, "cubic-link"+str(linkspeed));

# Cross-agility on delay
for delay in range(5, 151, 5):
  delay_topology = topofolder + "/delay" + str(delay) + ".txt"
  for run in range(1, iteration_count + 1):
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "1000x-delay"+str(delay));
    synthesize( "/home/am2/anirudh/bigbertha-100x.dna.5", delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "100x-delay"+str(delay));
    synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",  delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "10x-delay"+str(delay));
    synthesize( "NULL",                                   delay_topology,     cubicsfqCoDelstr, traffic_workload, 1.0, 100, run, "cubicsfqCoDel-delay"+str(delay));
    synthesize( "NULL",                                   delay_topology,     cubicstr,         traffic_workload, 1.0, 100, run, "cubic-delay"+str(delay));

# Cross-agility on duty cycle
for onpercent in range(5, 101, 5):
  dutycycle_topology = topofolder + "/dutycycle.txt"
  for run in range(1, iteration_count + 1):
    off_avg=( 100.0 - onpercent ) / onpercent;
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     dutycycle_topology, rationalstr,      traffic_workload, off_avg, 100, run, "1000x-dutycycle"+str(onpercent));
    synthesize( "/home/am2/anirudh/bigbertha-100x.dna.5", dutycycle_topology, rationalstr,      traffic_workload, off_avg, 100, run, "100x-dutycycle"+str(onpercent));
    synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",  dutycycle_topology, rationalstr,      traffic_workload, off_avg, 100, run, "10x-dutycycle"+str(onpercent));
    synthesize( "NULL",                                   dutycycle_topology, cubicsfqCoDelstr, traffic_workload, off_avg, 100, run, "cubicsfqCoDel-dutycycle"+str(onpercent));
    synthesize( "NULL",                                   dutycycle_topology, cubicstr,         traffic_workload, off_avg, 100, run, "cubic-dutycycle"+str(onpercent));

print "all: " + synthesize.targets, synthesize.cmdlines
