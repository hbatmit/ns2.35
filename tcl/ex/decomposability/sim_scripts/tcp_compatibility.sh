#! /bin/bash
set -x
rm /tmp/newreno.alwayson
rm /tmp/newreno.5sec
rm /tmp/baseline.alwayson
rm /tmp/baseline.5sec
rm /tmp/antagonize.alwayson
rm /tmp/antagonize.5sec

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Newreno -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/newreno.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Newreno -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/newreno.5sec
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/baseline.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=ratsonlybaselineV4.dna.6 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/baseline.5sec
  run=`expr $run '+' 1`
done
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV4.dna.4 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 0.01 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/antagonize.alwayson
  run=`expr $run '+' 1`
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=tcpantagonizeV4.dna.4 ./decompose.tcl topocompat.txt sdcompat.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 5.0 -offrand Exponential -offavg 5.0 -simtime 100 -run $run 2>&1 | grep mbps >> /tmp/antagonize.5sec
  run=`expr $run '+' 1`
done
