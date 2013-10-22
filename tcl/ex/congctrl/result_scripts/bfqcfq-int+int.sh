#! /bin/bash
# Compare CoDel+FQ with Bufferbloat+FQ
# Workload: Interactive+Interactive on 15 mbps link.

./remove-vestiges.sh
cd ..
seed=1

pls ./longrunning_sims.tcl remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -nsrc 2 -seed $seed -simtime 250 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 1000.0 -rttlog $seed.droptail.rtt -maxbins 1024 > $seed.droptail &
pls ./longrunning_sims.tcl remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -nsrc 2 -seed $seed -simtime 250 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005  -rttlog $seed.codel.rtt   -maxbins 1024  > $seed.codel &
