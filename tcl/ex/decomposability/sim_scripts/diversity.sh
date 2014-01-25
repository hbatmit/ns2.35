#! /bin/bash
rm -rf /tmp/independent-homogenous
mkdir /tmp/independent-homogenous
rm -rf /tmp/coevolved-homogenous
mkdir /tmp/coevolved-homogenous

rm -rf /tmp/independent-heterogenous
mkdir  /tmp/independent-heterogenous
rm -rf /tmp/coevolved-heterogenous
mkdir /tmp/coevolved-heterogenous

set -x
# Homogenous performance of independently evolved RemyCCs
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-0.1.dna.5 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/independent-homogenous/delta-0.1.2senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-10.0.dna.5 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/independent-homogenous/delta-10.0.2senders
  run=`expr $run '+' 1`;
done

# Homogenous performance of coevolved RemyCCs
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree1 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-homogenous/coevolved-delta-0.1.2senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree2 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-homogenous/coevolved-delta-10.0.2senders
  run=`expr $run '+' 1`;
done

# Heterogenous performance of independently evolved RemyCCs
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-0.1.dna.5 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run -altwhisk delta-10.0.dna.5 2>&1  | grep mbps >> /tmp/independent-heterogenous/independent-2senders
  run=`expr $run '+' 1`;
done

# Heterogenous performance of coevolved RemyCCs
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree1 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run  -altwhisk coexistencev2.dna.3.whiskertree2 2>&1  | grep mbps >> /tmp/coevolved-heterogenous/coevolved-2senders
  run=`expr $run '+' 1`;
done

set +x
cat /tmp/independent-homogenous/delta-0.1.2senders  | python sim_scripts/analyze_diversity.py "*" "independent-homogenous-delta-0.1"
cat /tmp/independent-homogenous/delta-10.0.2senders | python sim_scripts/analyze_diversity.py "*" "independent-homogenous-delta-10.0"
cat /tmp/coevolved-homogenous/coevolved-delta-0.1.2senders  | python sim_scripts/analyze_diversity.py "*" "coevolved-homogenous-delta-0.1"
cat /tmp/coevolved-homogenous/coevolved-delta-10.0.2senders | python sim_scripts/analyze_diversity.py "*" "coevolved-homogenous-delta-10.0"

cat /tmp/independent-heterogenous/independent-2senders | python sim_scripts/analyze_diversity.py "0" "independent-heterogenous-delta-0.1"
cat /tmp/independent-heterogenous/independent-2senders | python sim_scripts/analyze_diversity.py "1" "independent-heterogenous-delta-10.0"
cat /tmp/coevolved-heterogenous/coevolved-2senders | python sim_scripts/analyze_diversity.py "0" "coevolved-heterogenous-delta-0.1"
cat /tmp/coevolved-heterogenous/coevolved-2senders | python sim_scripts/analyze_diversity.py "1" "coevolved-heterogenous-delta-10.0"
