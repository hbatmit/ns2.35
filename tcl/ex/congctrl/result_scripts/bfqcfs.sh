#! /bin/bash
# Compare Bufferbloat+FQ with CoDel+FCFS
# Workload: one long running TCP that is interested in
# either: throughput/delay or throughput.

killall -s9 ns
rm *.log
rm *.codel
rm *.droptail
rm *.codelfcfs
pls ./remy2.tcl remyconf/vz4gdown.tcl -nsrc 1 -seed 1 -simtime 250 -reset false -avgbytes 1000000000 -offavg 0.0 -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 10000000 -pktsize 1210 -rcvwin 10000000 -delay 0.150 -rttlog newreno.rttcodel > newreno.codel &
pls ./remy2.tcl remyconf/vz4gdown.tcl -nsrc 1 -seed 1 -simtime 250 -reset false -avgbytes 1000000000 -offavg 0.0 -gw DropTail -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 10000000 -pktsize 1210 -rcvwin 10000000 -delay 0.150 -rttlog newreno.rttdroptail > newreno.droptail &
