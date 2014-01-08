#! /usr/bin/python
import os

# protocols
rationalstr="-tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail"
cubicstr="-tcp TCP/Linux/cubic -sink TCPSink/Sack1 -gw DropTail"
cubicsfqCoDelstr="-tcp TCP/Linux/cubic -sink TCPSink/Sack1 -gw sfqCoDel"

# traffic pattern
traffic_workload="-ontype time -onrand Exponential -onavg 1.0 -offrand Exponential"

# Clean up the results and topologies folder
os.system("rm -rf results")
os.system("mkdir results")
os.system("rm -rf topologies")
os.system("mkdir topologies")

# Make linkspeed topologies
for linkspeed in range(10, 1001, 10):
  fh=open("topologies/linkspeed"+ str(linkspeed) + ".txt", "w");
  fh.write("0 1 " + str(linkspeed) + " 74\n");
  fh.write("1 2 1000000000 1\n");
  fh.write("1 3 1000000000 1\n");

# Make delay toplogies
for delay in range(5, 151, 5):
  fh=open("topologies/delay"+ str(delay) + ".txt", "w");
  fh.write("0 1 500 " + str(delay) + "\n");
  fh.write("1 2 1000000000 1\n");
  fh.write("1 3 1000000000 1\n");

# Make common sd file
fh=open("topologies/sd.txt", 'w');
fh.write("0 2\n");
fh.write("0 3\n");

# Synthesize command line
def synthesize( whiskertree, topology, tcp_agents, traffic_cfg, off_time, sim_time, run, tag ):
  cmdline="WHISKERS=" + whiskertree + " pls ./decompose.tcl " + topology + " topologies/sd.txt " + tcp_agents + " " +  traffic_cfg + " -offavg "+ str( off_time ) + " -simtime " + str( sim_time ) + " -run " + str( run )
  fileio=" >results/" + tag + "run" + str( run ) + ".out " + "2>results/" + tag + "run" + str( run ) +  ".err"
  cmdline += fileio
  target = "results/" + tag + "run" + str( run ) + ".out"
  synthesize.targets += " " + target
  synthesize.cmdlines += "\n" + target + ":\n\t" + cmdline
synthesize.targets=""
synthesize.cmdlines=""

# Auto-agility
for linkspeed in range(10, 1001, 10):
  linkspeed_topology="topologies/linkspeed" + str(linkspeed) + ".txt"
  for run in range(1, 31):
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "1000x-link"+str(linkspeed));
    synthesize( "/home/am2/anirudh/bigbertha-100x.dna.5", linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "100x-link"+str(linkspeed));
    synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",  linkspeed_topology, rationalstr,      traffic_workload, 1.0, 100, run, "10x-link"+str(linkspeed));
    synthesize( "NULL",                                   linkspeed_topology, cubicsfqCoDelstr, traffic_workload, 1.0, 100, run, "cubicsfqCoDel-link"+str(linkspeed));
    synthesize( "NULL",                                   linkspeed_topology, cubicstr,         traffic_workload, 1.0, 100, run, "cubic-link"+str(linkspeed));

# Cross-agility on delay
for delay in range(5, 151, 5):
  delay_topology = "topologies/delay" + str(delay) + ".txt"
  for run in range(1, 31):
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "1000x-delay"+str(delay));
    synthesize( "/home/am2/anirudh/bigbertha-100x.dna.5", delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "100x-delay"+str(delay));
    synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",  delay_topology,     rationalstr,      traffic_workload, 1.0, 100, run, "10x-delay"+str(delay));
    synthesize( "NULL",                                   delay_topology,     cubicsfqCoDelstr, traffic_workload, 1.0, 100, run, "cubicsfqCoDel-delay"+str(delay));
    synthesize( "NULL",                                   delay_topology,     cubicstr,         traffic_workload, 1.0, 100, run, "cubic-delay"+str(delay));

# Cross-agility on duty cycle
for onpercent in range(5, 101, 5):
  dutycycle_topology="topologies/linkspeed500.txt"
  for run in range(1, 31):
    off_avg=( 100.0 - onpercent ) / onpercent;
    synthesize( "/home/am2/anirudh/bigbertha2.dna.5",     dutycycle_topology, rationalstr,      traffic_workload, off_avg, 100, run, "1000x-dutycycle"+str(onpercent));
    synthesize( "/home/am2/anirudh/bigbertha-100x.dna.5", dutycycle_topology, rationalstr,      traffic_workload, off_avg, 100, run, "100x-dutycycle"+str(onpercent));
    synthesize( "/home/am2/anirudh/bigbertha-10x.dna.4",  dutycycle_topology, rationalstr,      traffic_workload, off_avg, 100, run, "10x-dutycycle"+str(onpercent));
    synthesize( "NULL",                                   dutycycle_topology, cubicsfqCoDelstr, traffic_workload, off_avg, 100, run, "cubicsfqCoDel-dutycycle"+str(onpercent));
    synthesize( "NULL",                                   dutycycle_topology, cubicstr,         traffic_workload, off_avg, 100, run, "cubic-dutycycle"+str(onpercent));

print "all: " + synthesize.targets, synthesize.cmdlines
