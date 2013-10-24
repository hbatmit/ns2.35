#! /bin/bash
# Compare CoDel+FCFS with CoDel+FQ
# Workload: Bulk+Web on 15 mbps.

./remove-vestiges.sh
cd ..
seed=1

while [ $seed -lt 75 ]; do

  ./mixed_sims.tcl remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -nsrc 2 -seed $seed -simtime 1000 -ontype flowcdf -offtime 0.2 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005 -rttlog $seed.codelfcfs.rtt -maxbins 1 > $seed.codelfcfs &
  ./mixed_sims.tcl remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -nsrc 2 -seed $seed -simtime 1000 -ontype flowcdf -offtime 0.2 -reset false -verbose true -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005 -rttlog $seed.codel.rtt -maxbins 1024   > $seed.codel &

  seed=`expr $seed '+' 1`
done
wait
