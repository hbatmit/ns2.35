#! /bin/bash
set -x
rm /tmp/baseline.alwayson
rm /tmp/baseline.1sec
rm /tmp/antagonize.alwayson
rm /tmp/antagonize.1sec

WHISKERS=ratsalone-baselineV2.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 100000000000.0 -offrand Exponential -offavg 0.0 -simtime 100 -run 1 2>&1 | grep mbps > /tmp/baseline.alwayson

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsalone-baselineV2.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/baseline.1sec
  run=`expr $run '+' 1`
done

WHISKERS=tcpantagonizeV2.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 100000000000.0 -offrand Exponential -offavg 0.0 -simtime 100 -run 1 2>&1 | grep mbps > /tmp/antagonize.alwayson

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV2.dna.3 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/antagonize.1sec
  run=`expr $run '+' 1`
done
