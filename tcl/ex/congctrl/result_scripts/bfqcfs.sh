#! /bin/bash
# Compare Bufferbloat+FQ with CoDel+FCFS
# Workload: one long running TCP that is interested in
# either: throughput/delay or throughput.

./remove-vestiges.sh
cd ..
seed=1

pls ./remy2.tcl remyconf/vz4gdown.tcl -nsrc 1 -seed $seed -simtime 250 -reset false -avgbytes 1000000000 -offavg 0.0 -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 10000000 -pktsize 1210 -rcvwin 10000000 -delay 0.150 -rttlog $seed.codel.rtt > $seed.codel &
pls ./remy2.tcl remyconf/vz4gdown.tcl -nsrc 1 -seed $seed -simtime 250 -reset false -avgbytes 1000000000 -offavg 0.0 -gw DropTail -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 10000000 -pktsize 1210 -rcvwin 10000000 -delay 0.150 -rttlog $seed.droptail.rtt > $seed.droptail &
wait
