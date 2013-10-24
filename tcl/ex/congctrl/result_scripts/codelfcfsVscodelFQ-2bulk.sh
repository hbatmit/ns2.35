#! /bin/bash
# Compare CoDel+FCFS with CoDel+FQ
# Workload: 2 Bulk on v4g.

./remove-vestiges.sh
cd ..
seed=1

./longrunning_sims.tcl remyconf/vz4gdown.tcl -delay 0.150 -nsrc 2 -seed $seed -simtime 1000 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005 -rttlog $seed.codelfcfs.rtt -maxbins 1 > $seed.codelfcfs &
./longrunning_sims.tcl remyconf/vz4gdown.tcl -delay 0.150 -nsrc 2 -seed $seed -simtime 1000 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005 -rttlog $seed.codel.rtt -maxbins 1024   > $seed.codel &
wait
