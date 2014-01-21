#! /bin/bash

# Homogenous performance of independently evolved RemyCCs
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-0.1.dna.5 ./decompose.tcl topotest.txt sd1.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/delta-0.1.1sender
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-0.1.dna.5 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/delta-0.1.2senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-0.1.dna.5 ./decompose.tcl topotest.txt sd10.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/delta-0.1.10senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-10.0.dna.5 ./decompose.tcl topotest.txt sd1.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/delta-10.0.1sender
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-10.0.dna.5 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/delta-10.0.2senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=delta-10.0.dna.5 ./decompose.tcl topotest.txt sd10.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/delta-10.0.10senders
  run=`expr $run '+' 1`;
done

# Homogenous performance of coevolved RemyCCs
run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree1 ./decompose.tcl topotest.txt sd1.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-delta-0.1.1sender
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree1 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-delta-0.1.2senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree1 ./decompose.tcl topotest.txt sd10.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-delta-0.1.10senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree2 ./decompose.tcl topotest.txt sd1.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-delta-10.0.1sender
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree2 ./decompose.tcl topotest.txt sd2.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-delta-10.0.2senders
  run=`expr $run '+' 1`;
done

run=1;
while [ $run -lt 11 ]; do
  WHISKERS=coexistencev2.dna.3.whiskertree2 ./decompose.tcl topotest.txt sd10.txt -tcp TCP/Rational -sink TCPSink/Sack1 -gw DropTail -ontype time -onrand Exponential -onavg 1.0 -offrand Exponential -offavg 1.0 -simtime 100 -run $run 2>&1  | grep mbps >> /tmp/coevolved-delta-10.0.10senders
  run=`expr $run '+' 1`;
done
