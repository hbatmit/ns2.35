#! /bin/bash
# Compare CoDel+FQ with Bufferbloat+FQ
# Workload: Bulk+Web on v4g.

./remove-vestiges.sh
seed=1

while [ $seed -lt 75 ]; do

  pls ./mixed_sims.tcl remyconf/vz4gdown.tcl -delay 0.150 -nsrc 2 -seed $seed -simtime 250 -reset false -verbose true -ontype flowcdf  -offavg 0.2 -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 1000.0 -maxbins 1024 -rttlog $seed.droptail.rtt > $seed.droptail &
  pls ./mixed_sims.tcl remyconf/vz4gdown.tcl -delay 0.150 -nsrc 2 -seed $seed -simtime 250 -reset false -verbose true -ontype flowcdf  -offavg 0.2 -gw sfqCoDel -tcp TCP/Newreno -sink TCPSink/Sack1 -maxq 100000000 -pktsize 1210 -rcvwin 10000000 -flowoffset 16384 -codel_target 0.005  -maxbins 1024 -rttlog $seed.codel.rtt  > $seed.codel &

  seed=`expr $seed '+' 1`
done
