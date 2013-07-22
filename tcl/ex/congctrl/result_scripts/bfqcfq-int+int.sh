#! /bin/bash
# Compare CoDel+FQ with Bufferbloat+FQ
# Workload: Interactive+Interactive on 15 mbps link.

killall -s9 ns
rm *.log
rm *.codel
rm *.droptail
rm *.rtt

pls ./longrunning_sims.tcl remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -nsrc 2 -seed 1 -simtime 250 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 1000.0 -rttlog 2flow$seed.droptail.rtt -maxbins 1024 > 2flow$seed.droptail &
pls ./longrunning_sims.tcl remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -nsrc 2 -seed 1 -simtime 250 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005  -rttlog 2flow$seed.codel.rtt   -maxbins 1024  > 2flow$seed.codel &
